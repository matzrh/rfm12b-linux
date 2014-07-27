#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x8733bf46, "module_layout" },
	{ 0xadb5559d, "param_ops_byte" },
	{ 0x5d9b38ec, "no_llseek" },
	{ 0x38ef2151, "driver_unregister" },
	{ 0x21184c65, "class_destroy" },
	{ 0xa7cf41ce, "spi_register_driver" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x72f516de, "__class_create" },
	{ 0x4362204e, "__register_chrdev" },
	{ 0xd9339b7e, "irq_get_irq_data" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xe38a602d, "gpiochip_find" },
	{ 0x72542c85, "__wake_up" },
	{ 0xf087137d, "__dynamic_pr_debug" },
	{ 0x1c132024, "request_any_context_irq" },
	{ 0xf3f97e1, "nonseekable_open" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0x311b7963, "_raw_spin_unlock" },
	{ 0xc2161e33, "_raw_spin_lock" },
	{ 0x1ff8c39c, "spi_async" },
	{ 0x8834396c, "mod_timer" },
	{ 0xbe2c0274, "add_timer" },
	{ 0x7d11c268, "jiffies" },
	{ 0xfb0e29f, "init_timer_key" },
	{ 0xc996d097, "del_timer" },
	{ 0xfe990052, "gpio_free" },
	{ 0x3ce4ca6f, "disable_irq" },
	{ 0xf20dabd8, "free_irq" },
	{ 0xe4f1beaa, "spi_add_device" },
	{ 0x73e20c1c, "strlcpy" },
	{ 0xef73ede7, "bus_find_device_by_name" },
	{ 0xb81960ca, "snprintf" },
	{ 0x4345034a, "spi_alloc_device" },
	{ 0xb0aeb308, "spi_busnum_to_master" },
	{ 0x6b1e13f2, "put_device" },
	{ 0x3794514f, "device_unregister" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xf83178bd, "finish_wait" },
	{ 0x32f80ea9, "prepare_to_wait" },
	{ 0x1000e51, "schedule" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0x99bb8806, "memmove" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x5f754e5a, "memset" },
	{ 0xf9a482f9, "msleep" },
	{ 0x1383046b, "spi_sync" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x2196324, "__aeabi_idiv" },
	{ 0x343a1a8, "__list_add" },
	{ 0x676bbc0f, "_set_bit" },
	{ 0x95284ee, "__dynamic_dev_dbg" },
	{ 0x9bf1d8c2, "device_create" },
	{ 0xd3dbfbc4, "_find_first_zero_bit_le" },
	{ 0x4467122a, "__init_waitqueue_head" },
	{ 0xfa1d2994, "kmem_cache_alloc" },
	{ 0x5abec8b0, "kmalloc_caches" },
	{ 0xc8b30bd9, "mutex_unlock" },
	{ 0x49ebacbd, "_clear_bit" },
	{ 0xf56a7fe0, "device_destroy" },
	{ 0x521445b, "list_del" },
	{ 0x1ffd893d, "mutex_lock" },
	{ 0xa48257, "dev_set_drvdata" },
	{ 0x27e1a049, "printk" },
	{ 0x1d45b6f1, "dev_get_drvdata" },
	{ 0x37a0cba, "kfree" },
	{ 0x74c97f9c, "_raw_spin_unlock_irqrestore" },
	{ 0xbd7083bc, "_raw_spin_lock_irqsave" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "44E3FF52709BD5C1B7852E1");
