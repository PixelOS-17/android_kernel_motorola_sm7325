# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for cpuidle governors.
#

obj-$(CONFIG_CPU_IDLE_GOV_LADDER) += ladder.o
obj-$(CONFIG_CPU_IDLE_GOV_MENU) += menu.o
obj-$(CONFIG_CPU_IDLE_GOV_TEO) += teo.o
obj-$(CONFIG_CPU_IDLE_GOV_HALTPOLL) += haltpoll.o

# tundra (from seventeen-6.1)
CFLAGS_qcom_lpm.o := -I$(src)
qcom_simple_lpm-y += qcom-simple-lpm.o
qcom_simple_lpm-y += qcom-simple-cluster-lpm.o
qcom_simple_lpm-y += qcom-simple-lpm-sysfs.o
qcom_lpm-y += qcom-lpm.o
qcom_lpm-y += qcom-cluster-lpm.o
qcom_lpm-y += qcom-lpm-sysfs.o
