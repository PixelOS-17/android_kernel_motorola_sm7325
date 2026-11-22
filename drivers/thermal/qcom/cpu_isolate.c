// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched/walt.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/workqueue.h>

#define CPU_ISOLATE_LEVEL 1

struct cpu_isolate_cdev {
	struct list_head node;
	int cpu_id;
	bool cpu_isolate_state;
	struct thermal_cooling_device *cdev;
	struct device_node *np;
	char cdev_name[THERMAL_NAME_LENGTH];
	struct work_struct reg_work;
};

static enum cpuhp_state cpu_hp_online;
static DEFINE_MUTEX(cpu_isolate_lock);
static LIST_HEAD(cpu_isolate_cdev_list);

static int cpu_isolate_hp_online(unsigned int online_cpu)
{
	struct cpu_isolate_cdev *cpu_isolate_cdev;

	mutex_lock(&cpu_isolate_lock);
	list_for_each_entry(cpu_isolate_cdev, &cpu_isolate_cdev_list, node) {
		if (online_cpu != cpu_isolate_cdev->cpu_id)
			continue;

		if (!cpu_isolate_cdev->cdev)
			queue_work(system_highpri_wq, &cpu_isolate_cdev->reg_work);
		break;
	}
	mutex_unlock(&cpu_isolate_lock);

	return 0;
}

static int cpu_isolate_set_cur_state(struct thermal_cooling_device *cdev,
				     unsigned long state)
{
	struct cpu_isolate_cdev *cpu_isolate_cdev = cdev->devdata;
	cpumask_t mask;
	int ret = 0;

	if (cpu_isolate_cdev->cpu_id == -1)
		return -ENODEV;

	if (state > CPU_ISOLATE_LEVEL)
		return -EINVAL;

	state = !!state;

	mutex_lock(&cpu_isolate_lock);
	if (cpu_isolate_cdev->cpu_isolate_state == state)
		goto unlock;

	/* walt_pause_cpus()/walt_resume_cpus() mutate the passed cpumask. */
	cpumask_clear(&mask);
	cpumask_set_cpu(cpu_isolate_cdev->cpu_id, &mask);

	if (state == CPU_ISOLATE_LEVEL)
		ret = walt_pause_cpus(&mask, PAUSE_THERMAL);
	else
		ret = walt_resume_cpus(&mask, PAUSE_THERMAL);

	if (!ret)
		cpu_isolate_cdev->cpu_isolate_state = state;
unlock:
	mutex_unlock(&cpu_isolate_lock);

	return ret;
}

static int cpu_isolate_get_cur_state(struct thermal_cooling_device *cdev,
				     unsigned long *state)
{
	struct cpu_isolate_cdev *cpu_isolate_cdev = cdev->devdata;

	mutex_lock(&cpu_isolate_lock);
	*state = cpu_isolate_cdev->cpu_isolate_state ? CPU_ISOLATE_LEVEL : 0;
	mutex_unlock(&cpu_isolate_lock);

	return 0;
}

static int cpu_isolate_get_max_state(struct thermal_cooling_device *cdev,
				     unsigned long *state)
{
	*state = CPU_ISOLATE_LEVEL;
	return 0;
}

static const struct thermal_cooling_device_ops cpu_isolate_cooling_ops = {
	.get_max_state = cpu_isolate_get_max_state,
	.get_cur_state = cpu_isolate_get_cur_state,
	.set_cur_state = cpu_isolate_set_cur_state,
};

static void cpu_isolate_register_cdev(struct work_struct *work)
{
	struct cpu_isolate_cdev *cpu_isolate_cdev =
		container_of(work, struct cpu_isolate_cdev, reg_work);
	struct thermal_cooling_device *cdev;

	mutex_lock(&cpu_isolate_lock);
	if (cpu_isolate_cdev->cdev) {
		mutex_unlock(&cpu_isolate_lock);
		return;
	}
	mutex_unlock(&cpu_isolate_lock);

	cdev = thermal_of_cooling_device_register(cpu_isolate_cdev->np,
						  cpu_isolate_cdev->cdev_name,
						  cpu_isolate_cdev,
						  &cpu_isolate_cooling_ops);
	if (IS_ERR(cdev)) {
		pr_err("Cooling register failed for %s, ret:%ld\n",
		       cpu_isolate_cdev->cdev_name, PTR_ERR(cdev));
		return;
	}

	mutex_lock(&cpu_isolate_lock);
	if (cpu_isolate_cdev->cdev) {
		mutex_unlock(&cpu_isolate_lock);
		thermal_cooling_device_unregister(cdev);
		return;
	}
	cpu_isolate_cdev->cdev = cdev;
	mutex_unlock(&cpu_isolate_lock);
	pr_debug("Cooling device [%s] registered.\n",
		 cpu_isolate_cdev->cdev_name);
}

static int cpu_isolate_probe(struct platform_device *pdev)
{
	int ret = 0, cpu;
	struct device_node *dev_phandle, *subsys_np;
	struct device *cpu_dev;
	struct cpu_isolate_cdev *cpu_isolate_cdev;
	struct device_node *np = pdev->dev.of_node;

	INIT_LIST_HEAD(&cpu_isolate_cdev_list);
	for_each_available_child_of_node(np, subsys_np) {
		cpu_isolate_cdev = devm_kzalloc(&pdev->dev,
						sizeof(*cpu_isolate_cdev),
						GFP_KERNEL);
		if (!cpu_isolate_cdev) {
			of_node_put(subsys_np);
			return -ENOMEM;
		}
		cpu_isolate_cdev->cpu_id = -1;

		dev_phandle = of_parse_phandle(subsys_np, "qcom,cpu", 0);
		for_each_possible_cpu(cpu) {
			cpu_dev = get_cpu_device(cpu);
			if (cpu_dev && cpu_dev->of_node == dev_phandle) {
				cpu_isolate_cdev->cpu_id = cpu;
				break;
			}
		}
		of_node_put(dev_phandle);

		if (cpu_isolate_cdev->cpu_id == -1) {
			dev_err(&pdev->dev, "Invalid CPU phandle\n");
			continue;
		}

		cpu_isolate_cdev->np = of_node_get(subsys_np);
		snprintf(cpu_isolate_cdev->cdev_name, THERMAL_NAME_LENGTH,
			 "cpu-isolate%d", cpu_isolate_cdev->cpu_id);
		INIT_WORK(&cpu_isolate_cdev->reg_work,
			  cpu_isolate_register_cdev);
		list_add(&cpu_isolate_cdev->node, &cpu_isolate_cdev_list);
	}

	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "cpu-isolate/cdev:online",
				cpu_isolate_hp_online, NULL);
	if (ret < 0) {
		struct cpu_isolate_cdev *next;

		list_for_each_entry_safe(cpu_isolate_cdev, next,
					 &cpu_isolate_cdev_list, node) {
			of_node_put(cpu_isolate_cdev->np);
			list_del(&cpu_isolate_cdev->node);
		}
		return ret;
	}
	cpu_hp_online = ret;

	return 0;
}

static void cpu_isolate_remove(struct platform_device *pdev)
{
	struct cpu_isolate_cdev *cpu_isolate_cdev, *next;
	cpumask_t mask;

	if (cpu_hp_online) {
		cpuhp_remove_state_nocalls(cpu_hp_online);
		cpu_hp_online = 0;
	}

	list_for_each_entry(cpu_isolate_cdev, &cpu_isolate_cdev_list, node)
		cancel_work_sync(&cpu_isolate_cdev->reg_work);

	mutex_lock(&cpu_isolate_lock);
	list_for_each_entry_safe(cpu_isolate_cdev, next,
				 &cpu_isolate_cdev_list, node) {
		if (cpu_isolate_cdev->cdev)
			thermal_cooling_device_unregister(cpu_isolate_cdev->cdev);
		if (cpu_isolate_cdev->cpu_isolate_state) {
			cpumask_clear(&mask);
			cpumask_set_cpu(cpu_isolate_cdev->cpu_id, &mask);
			walt_resume_cpus(&mask, PAUSE_THERMAL);
			cpu_isolate_cdev->cpu_isolate_state = false;
		}
		of_node_put(cpu_isolate_cdev->np);
		list_del(&cpu_isolate_cdev->node);
	}
	mutex_unlock(&cpu_isolate_lock);
}

static const struct of_device_id cpu_isolate_match[] = {
	{ .compatible = "qcom,cpu-isolate", },
	{},
};
MODULE_DEVICE_TABLE(of, cpu_isolate_match);

static struct platform_driver cpu_isolate_driver = {
	.probe		= cpu_isolate_probe,
	.remove		= cpu_isolate_remove,
	.driver		= {
		.name = KBUILD_MODNAME,
		.of_match_table = cpu_isolate_match,
	},
};
module_platform_driver(cpu_isolate_driver);
MODULE_DESCRIPTION("CPU Isolate cooling device driver");
MODULE_LICENSE("GPL");
