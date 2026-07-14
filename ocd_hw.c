#include "our_cam_core.h"
#include <linux/interrupt.h>
#include <linux/io.h>

irqreturn_t our_cam_isr(int irq, void *priv)
{
	struct our_cam_dev *dev = priv;
	struct our_cam_cap_dev *cap = &dev->cap;
	struct our_cam_buffer *buf;
	unsigned long flags;

	/* * TODO: 
	 * 1. Read hardware status register
	 * 2. Clear interrupt flag
	 * 3. Return IRQ_NONE if not our interrupt
	 */

	spin_lock_irqsave(&cap->slock, flags);

	/* Frame completion logic */
	if (!list_empty(&cap->active_bufs)) {
		buf = list_first_entry(&cap->active_bufs, struct our_cam_buffer, list);
		list_del(&buf->list);
		
		buf->vbuf.vb2_buf.timestamp = ktime_get_ns();
		buf->vbuf.sequence = cap->sequence++;
		vb2_buffer_done(&buf->vbuf.vb2_buf, VB2_BUF_STATE_DONE);
	}

	/* Queue next buffer to hardware if available */
	if (!list_empty(&cap->pending_bufs)) {
		buf = list_first_entry(&cap->pending_bufs, struct our_cam_buffer, list);
		list_move_tail(&buf->list, &cap->active_bufs);
		
		/* TODO: writel(buf->dma_addr, dev->regs + HW_DMA_ADDR_REG); */
	}

	spin_unlock_irqrestore(&cap->slock, flags);

	return IRQ_HANDLED;
}

int our_cam_hw_init(struct our_cam_dev *dev)
{
	/* * TODO: 
	 * 1. Request IRQ line
	 * 2. Enable clocks
	 * 3. Reset hardware block
	 */
	 
	return 0;
}

void our_cam_hw_enable_streaming(struct our_cam_dev *dev, bool enable)
{
	/* * TODO: 
	 * Set/Clear run bit in hardware control register 
	 */
}
