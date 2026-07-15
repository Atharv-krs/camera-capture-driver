#include "ocd_core.h"
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-dma-contig.h>

/* --- Videobuf2 Operations --- */

static int our_cam_queue_setup(struct vb2_queue *q,
			      unsigned int *num_buffers, unsigned int *num_planes,
			      unsigned int sizes[], struct device *alloc_devs[])
{
	struct our_cam_cap_dev *cap = vb2_get_drv_priv(q);
	unsigned int size = cap->current_fmt.sizeimage;

	if (*num_planes)
		return sizes[0] < size ? -EINVAL : 0;

	*num_planes = 1;
	sizes[0] = size;
	alloc_devs[0] = cap->parent->dev;

	return 0;
}

static int our_cam_buf_prepare(struct vb2_buffer *vb)
{
	struct our_cam_cap_dev *cap = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long size = cap->current_fmt.sizeimage;

	if (vb2_plane_size(vb, 0) < size)
		return -EINVAL;

	vb2_set_plane_payload(vb, 0, size);
	return 0;
}

static void our_cam_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct our_cam_buffer *our_buf = container_of(vbuf, struct our_cam_buffer, vbuf);
	struct our_cam_cap_dev *cap = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long flags;

	our_buf->dma_addr = vb2_dma_contig_plane_dma_addr(vb, 0);

	spin_lock_irqsave(&cap->slock, flags);
	list_add_tail(&our_buf->list, &cap->pending_bufs);
	spin_unlock_irqrestore(&cap->slock, flags);
}

static int our_cam_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct our_cam_cap_dev *cap = vb2_get_drv_priv(q);
	
	cap->sequence = 0;
	our_cam_hw_enable_streaming(cap->parent, true);
	return 0;
}

static void our_cam_stop_streaming(struct vb2_queue *q)
{
	struct our_cam_cap_dev *cap = vb2_get_drv_priv(q);
	struct our_cam_buffer *buf, *tmp;
	unsigned long flags;

	our_cam_hw_enable_streaming(cap->parent, false);

	spin_lock_irqsave(&cap->slock, flags);
	list_for_each_entry_safe(buf, tmp, &cap->pending_bufs, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vbuf.vb2_buf, VB2_BUF_STATE_ERROR);
	}
	list_for_each_entry_safe(buf, tmp, &cap->active_bufs, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vbuf.vb2_buf, VB2_BUF_STATE_ERROR);
	}
	spin_unlock_irqrestore(&cap->slock, flags);
}

static const struct vb2_ops our_cam_vb2_ops = {
	.queue_setup     = our_cam_queue_setup,
	.buf_prepare     = our_cam_buf_prepare,
	.buf_queue       = our_cam_buf_queue,
	.start_streaming = our_cam_start_streaming,
	.stop_streaming  = our_cam_stop_streaming,
	.wait_prepare    = vb2_ops_wait_prepare,
	.wait_finish     = vb2_ops_wait_finish,
};

/* --- V4L2 IOCTLs --- */

static int vidioc_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	strscpy(cap->driver, OUR_CAM_DRV_NAME, sizeof(cap->driver));
	strscpy(cap->card, "Custom Camera Device", sizeof(cap->card));
	return 0;
}

static int vidioc_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct our_cam_cap_dev *cap = video_drvdata(file);
	f->fmt.pix = cap->current_fmt;
	return 0;
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;

	/* Dumour constraints for skeleton */
	pix->width = 640;
	pix->height = 480;
	pix->pixelformat = V4L2_PIX_FMT_YUYV;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = pix->width * 2;
	pix->sizeimage = pix->bytesperline * pix->height;
	pix->colorspace = V4L2_COLORSPACE_SRGB;

	return 0;
}

static int vidioc_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct our_cam_cap_dev *cap = video_drvdata(file);
	
	if (vb2_is_busy(&cap->vb2_q))
		return -EBUSY;

	vidioc_try_fmt_vid_cap(file, priv, f);
	cap->current_fmt = f->fmt.pix;

	return 0;
}

static const struct v4l2_ioctl_ops our_cam_ioctl_ops = {
	.vidioc_querycap       = vidioc_querycap,
	.vidioc_g_fmt_vid_cap  = vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap= vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap  = vidioc_s_fmt_vid_cap,
	.vidioc_reqbufs        = vb2_ioctl_reqbufs,
	.vidioc_querybuf       = vb2_ioctl_querybuf,
	.vidioc_qbuf           = vb2_ioctl_qbuf,
	.vidioc_dqbuf          = vb2_ioctl_dqbuf,
	.vidioc_streamon       = vb2_ioctl_streamon,
	.vidioc_streamoff      = vb2_ioctl_streamoff,
};

static const struct v4l2_file_operations our_cam_fops = {
	.owner          = THIS_MODULE,
	.open           = v4l2_fh_open,
	.release        = vb2_fop_release,
	.poll           = vb2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap           = vb2_fop_mmap,
};

/* --- Registration --- */

int our_cam_register_video(struct our_cam_dev *dev)
{
	struct our_cam_cap_dev *cap = &dev->cap;
	struct video_device *vdev = &cap->vdev;
	struct vb2_queue *q = &cap->vb2_q;
	int ret;

	cap->parent = dev;
	INIT_LIST_HEAD(&cap->pending_bufs);
	INIT_LIST_HEAD(&cap->active_bufs);
	mutex_init(&cap->lock);
	spin_lock_init(&cap->slock);

	/* Set a safe default format */
	cap->current_fmt.width = 640;
	cap->current_fmt.height = 480;
	cap->current_fmt.pixelformat = V4L2_PIX_FMT_YUYV;
	cap->current_fmt.bytesperline = 640 * 2;
	cap->current_fmt.sizeimage = 640 * 480 * 2;

	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_DMABUF;
	q->drv_priv = cap;
	q->ops = &our_cam_vb2_ops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->buf_struct_size = sizeof(struct our_cam_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = &cap->lock;
	q->dev = dev->dev;
	
	ret = vb2_queue_init(q);
	if (ret)
		return ret;

	snprintf(vdev->name, sizeof(vdev->name), "our_cam_cap");
	vdev->fops = &our_cam_fops;
	vdev->ioctl_ops = &our_cam_ioctl_ops;
	vdev->v4l2_dev = &dev->v4l2_dev;
	vdev->queue = q;
	vdev->lock = &cap->lock;
	vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
    vdev->release = video_device_release_empty;	
	video_set_drvdata(vdev, cap);

	return video_register_device(vdev, VFL_TYPE_VIDEO, -1);
}

void our_cam_unregister_video(struct our_cam_dev *dev)
{
	video_unregister_device(&dev->cap.vdev);
}
