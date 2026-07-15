#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include "ocd_core.h"

static int our_cam_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct our_cam_dev *our_cam;
	int ret;

	our_cam = devm_kzalloc(dev, sizeof(*our_cam), GFP_KERNEL);
	if (!our_cam)
		return -ENOMEM;

	our_cam->dev = dev;
	platform_set_drvdata(pdev, our_cam);

	/* 1. Map Hardware Registers */
	our_cam->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(our_cam->regs)) {
		/* Non-fatal for a pure skeleton without actual DT bindings yet */
		dev_warn(dev, "No IO memory resource found, continuing in dumour mode\n");
	}

	/* 2. Setup V4L2 Device (The parent of all media entities) */
	ret = v4l2_device_register(dev, &our_cam->v4l2_dev);
	if (ret) {
		dev_err(dev, "Failed to register v4l2_device\n");
		return ret;
	}

	/* 3. Initialize Hardware / IRQs */
	ret = our_cam_hw_init(our_cam);
	if (ret)
		goto err_v4l2;

	/* 4. Register Video Node (/dev/videoX) */
	ret = our_cam_register_video(our_cam);
	if (ret)
		goto err_v4l2;

	dev_info(dev, "Driver probed successfully\n");
	return 0;

err_v4l2:
	v4l2_device_unregister(&our_cam->v4l2_dev);
	return ret;
}

static int our_cam_remove(struct platform_device *pdev)
{
	struct our_cam_dev *our_cam = platform_get_drvdata(pdev);

	our_cam_unregister_video(our_cam);
	v4l2_device_unregister(&our_cam->v4l2_dev);
	
	dev_info(&pdev->dev, "Driver removed\n");
    return 0;
}

static const struct of_device_id our_cam_of_match[] = {
	{ .compatible = "custom,our-cam", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, our_cam_of_match);

static struct platform_driver our_cam_driver = {
	.probe  = our_cam_probe,
	.remove = our_cam_remove,
	.driver = {
		.name           = OUR_CAM_DRV_NAME,
		.of_match_table = our_cam_of_match,
	},
};

//uncomment hardware linkage and comment software mock in case of real hardware and vice-versa

/*hardware linkage*/

//module_platform_driver(our_cam_driver);

/*software mock of hardware*/
static struct platform_device *our_cam_pdev;

static int __init our_cam_init(void)
{
	int ret;

	ret = platform_driver_register(&our_cam_driver);
	if (ret)
		return ret;

	our_cam_pdev = platform_device_register_simple(OUR_CAM_DRV_NAME, -1, NULL, 0);
	if (IS_ERR(our_cam_pdev)) {
		platform_driver_unregister(&our_cam_driver);
		return PTR_ERR(our_cam_pdev);
	}

	return 0;
}

static void __exit our_cam_exit(void)
{
	platform_device_unregister(our_cam_pdev);
	platform_driver_unregister(&our_cam_driver);
}

module_init(our_cam_init);
module_exit(our_cam_exit);
/*mock finish*/

MODULE_AUTHOR("TEAM TAB");
MODULE_DESCRIPTION("Custom V4L2 Capture Skeleton Driver");
MODULE_LICENSE("GPL v2");
