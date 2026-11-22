// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016-2020, The Linux Foundation. All rights reserved.
 */
#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

static void lmh_isens_vref_disable(void *data)
{
	regulator_disable(data);
}

static int lmh_isens_vref_init_one(struct device *dev, const char *reg_name,
				   const char *setting_prop)
{
	struct regulator *reg;
	u32 settings[3];
	int ret;

	ret = of_property_read_u32_array(dev->of_node, setting_prop,
					 settings, ARRAY_SIZE(settings));
	if (ret == -EINVAL)
		return 0;
	if (ret) {
		dev_err(dev, "Failed to read %s: %d\n", setting_prop, ret);
		return ret;
	}

	reg = devm_regulator_get(dev, reg_name);
	if (IS_ERR(reg))
		return dev_err_probe(dev, PTR_ERR(reg),
				     "Failed to get %s regulator\n", reg_name);

	ret = regulator_set_voltage(reg, settings[0], settings[1]);
	if (ret) {
		dev_err(dev, "Failed to set %s voltage: %d\n", reg_name, ret);
		return ret;
	}

	ret = regulator_set_load(reg, settings[2]);
	if (ret) {
		dev_err(dev, "Failed to set %s load: %d\n", reg_name, ret);
		return ret;
	}

	ret = regulator_enable(reg);
	if (ret) {
		dev_err(dev, "Failed to enable %s: %d\n", reg_name, ret);
		return ret;
	}

	return devm_add_action_or_reset(dev, lmh_isens_vref_disable, reg);
}

static int lmh_isens_vref_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	ret = lmh_isens_vref_init_one(dev, "isens_vref_1p8",
				      "isens-vref-1p8-settings");
	if (ret)
		return ret;

	return lmh_isens_vref_init_one(dev, "isens_vref_0p8",
				       "isens-vref-0p8-settings");
}

static const struct of_device_id lmh_isens_vref_match[] = {
	{ .compatible = "qcom,msm-hw-limits" },
	{},
};
MODULE_DEVICE_TABLE(of, lmh_isens_vref_match);

static struct platform_driver lmh_isens_vref_driver = {
	.probe		= lmh_isens_vref_probe,
	.driver		= {
		.name = "msm_lmh_isens",
		.of_match_table = lmh_isens_vref_match,
	},
};
module_platform_driver(lmh_isens_vref_driver);
MODULE_DESCRIPTION("QTI LMh iSens VREF regulator init driver");
MODULE_LICENSE("GPL");
