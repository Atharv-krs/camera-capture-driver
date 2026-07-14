#ifndef __OUR_CAM_CORE_H__
#define __OUR_CAM_CORE_H__

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-v4l2.h>

#define OUR_CAM_DRV_NAME "our_cam"

/* Forward declaration */

struct our_cam_dev;

/**
 * struct our_cam_buffer - Custom vb2 buffer wrapper
 */
 
struct our_cam_buffer {
	struct vb2_v4l2_buffer vbuf;
	struct list_head list;
	dma_addr_t dma_addr;
};

/**
 * struct our_cam_cap_dev - Video device context (/dev/videoX)
 */
 
struct our_cam_cap_dev {
	struct video_device vdev;
	struct vb2_queue vb2_q;
	
	struct v4l2_pix_format current_fmt;
	unsigned int sequence;
	
	struct mutex lock;        /* Serializes file ops and ioctls */
	spinlock_t slock;         /* Protects the active buffer lists */
	
	struct list_head pending_bufs;
	struct list_head active_bufs;
	
	struct our_cam_dev *parent;
};

/**
 * struct our_cam_dev - Main hardware device context
 */
 
struct our_cam_dev {
	struct device *dev;
	struct v4l2_device v4l2_dev;
	struct v4l2_ctrl_handler ctrl_handler;
	
	void __iomem *regs;
	int irq;
	
	struct our_cam_cap_dev cap;
};

/* --- Sub-module Prototypes --- */

/* From our_cam_cap.c */

int our_cam_register_video(struct our_cam_dev *dev);
void our_cam_unregister_video(struct our_cam_dev *dev);

/* From our_cam_hw.c */

int our_cam_hw_init(struct our_cam_dev *dev);
void our_cam_hw_enable_streaming(struct our_cam_dev *dev, bool enable);
irqreturn_t our_cam_isr(int irq, void *priv);

#endif /* __OUR_CAM_CORE_H__ */
