// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
 */

#include "mtk_vcodec_drv.h"
#include "mtk_vcodec_util.h"
#include "vdec_drv_if.h"
#include "vdec_ipi_msg.h"
#include "vdec_vpu_if.h"
#include "mtk_vcodec_fw.h"

static void handle_init_ack_msg(const struct vdec_vpu_ipi_init_ack *msg)
{
	struct vdec_vpu_inst *vpu = (struct vdec_vpu_inst *)
					(unsigned long)msg->ap_inst_addr;

	mtk_vcodec_debug(vpu, "+ ap_inst_addr = 0x%llx", msg->ap_inst_addr);

	/* mapping VPU address to kernel virtual address */
	/* the content in vsi is initialized to 0 in VPU */
	vpu->vsi = mtk_vcodec_fw_map_dm_addr(vpu->ctx->dev->fw_handler,
					     msg->vpu_inst_addr);
	vpu->inst_addr = msg->vpu_inst_addr;

	mtk_vcodec_debug(vpu, "- vpu_inst_addr = 0x%x", vpu->inst_addr);

	/* Set default ABI version if dealing with unversioned firmware. */
	vpu->fw_abi_version = 0;
	/*
	 * Instance ID is only used if ABI version >= 2. Initialize it with
	 * garbage by default.
	 */
	vpu->inst_id = 0xdeadbeef;

	/* VPU firmware does not contain a version field. */
	if (mtk_vcodec_fw_get_type(vpu->ctx->dev->fw_handler) == VPU)
		return;

	/* Check firmware version. */
	vpu->fw_abi_version = msg->vdec_abi_version;
	mtk_vcodec_debug(vpu, "firmware version 0x%x\n", vpu->fw_abi_version);
	switch (vpu->fw_abi_version) {
	case 1:
		break;
	case 2:
		vpu->inst_id = msg->inst_id;
		break;
	default:
		mtk_vcodec_err(vpu, "unhandled firmware version 0x%x\n",
			       vpu->fw_abi_version);
		vpu->failure = 1;
		break;
	}
}

static void handle_get_param_msg_ack(const struct vdec_vpu_ipi_get_param_ack *msg)
{
	struct vdec_vpu_inst *vpu = (struct vdec_vpu_inst *)
					(unsigned long)msg->ap_inst_addr;

	mtk_vcodec_debug(vpu, "+ ap_inst_addr = 0x%llx", msg->ap_inst_addr);

	/* param_type is enum vdec_get_param_type */
	switch (msg->param_type) {
	case GET_PARAM_PIC_INFO:
		vpu->fb_sz[0] = msg->data[0];
		vpu->fb_sz[1] = msg->data[1];
		break;
	default:
		mtk_vcodec_err(vpu, "invalid get param type=%d", msg->param_type);
		vpu->failure = 1;
		break;
	}
}

/*
 * vpu_dec_ipi_handler - Handler for VPU ipi message.
 *
 * @data: ipi message
 * @len : length of ipi message
 * @priv: callback private data which is passed by decoder when register.
 *
 * This function runs in interrupt context and it means there's an IPI MSG
 * from VPU.
 */
static void vpu_dec_ipi_handler(void *data, unsigned int len, void *priv)
{
	const struct vdec_vpu_ipi_ack *msg = data;
	struct vdec_vpu_inst *vpu = (struct vdec_vpu_inst *)
					(unsigned long)msg->ap_inst_addr;

	if (!vpu) {
		mtk_v4l2_err("ap_inst_addr is NULL, did the SCP hang or crash?");
		return;
	}

	mtk_vcodec_debug(vpu, "+ id=%X", msg->msg_id);

	vpu->failure = msg->status;
	vpu->signaled = 1;

	if (msg->status == 0) {
		switch (msg->msg_id) {
		case VPU_IPIMSG_DEC_INIT_ACK:
			handle_init_ack_msg(data);
			break;

		case VPU_IPIMSG_DEC_START_ACK:
		case VPU_IPIMSG_DEC_END_ACK:
		case VPU_IPIMSG_DEC_DEINIT_ACK:
		case VPU_IPIMSG_DEC_RESET_ACK:
		case VPU_IPIMSG_DEC_CORE_ACK:
		case VPU_IPIMSG_DEC_CORE_END_ACK:
			break;

		case VPU_IPIMSG_DEC_GET_PARAM_ACK:
			handle_get_param_msg_ack(data);
			break;
		default:
			mtk_vcodec_err(vpu, "invalid msg=%X", msg->msg_id);
			break;
		}
	}

	mtk_vcodec_debug(vpu, "- id=%X", msg->msg_id);
}

static int vcodec_vpu_send_msg(struct vdec_vpu_inst *vpu, void *msg, int len)
{
	int err, id, msgid;

	msgid = *(uint32_t *)msg;
	mtk_vcodec_debug(vpu, "id=%X", msgid);

	vpu->failure = 0;
	vpu->signaled = 0;

	if (vpu->ctx->dev->vdec_pdata->hw_arch == MTK_VDEC_LAT_SINGLE_CORE) {
		if (msgid == AP_IPIMSG_DEC_CORE ||
		    msgid == AP_IPIMSG_DEC_CORE_END)
			id = vpu->core_id;
		else
			id = vpu->id;
	} else {
		id = vpu->id;
	}

	err = mtk_vcodec_fw_ipi_send(vpu->ctx->dev->fw_handler, id, msg,
				     len, 2000);
	if (err) {
		mtk_vcodec_err(vpu, "send fail vpu_id=%d msg_id=%X status=%d",
			       id, msgid, err);
		return err;
	}

	return vpu->failure;
}

static int vcodec_send_ap_ipi(struct vdec_vpu_inst *vpu, unsigned int msg_id)
{
	struct vdec_ap_ipi_cmd msg;
	int err = 0;

	mtk_vcodec_debug(vpu, "+ id=%X", msg_id);

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = msg_id;
	if (vpu->fw_abi_version < 2)
		msg.vpu_inst_addr = vpu->inst_addr;
	else
		msg.inst_id = vpu->inst_id;
	msg.codec_type = vpu->codec_type;

	err = vcodec_vpu_send_msg(vpu, &msg, sizeof(msg));
	mtk_vcodec_debug(vpu, "- id=%X ret=%d", msg_id, err);
	return err;
}

int vpu_dec_init(struct vdec_vpu_inst *vpu)
{
	struct vdec_ap_ipi_init msg;
	int err;

	mtk_vcodec_debug_enter(vpu);

	init_waitqueue_head(&vpu->wq);
	vpu->handler = vpu_dec_ipi_handler;

	err = mtk_vcodec_fw_ipi_register(vpu->ctx->dev->fw_handler, vpu->id,
					 vpu->handler, "vdec", NULL);
	if (err) {
		mtk_vcodec_err(vpu, "vpu_ipi_register fail status=%d", err);
		return err;
	}

	if (vpu->ctx->dev->vdec_pdata->hw_arch == MTK_VDEC_LAT_SINGLE_CORE) {
		err = mtk_vcodec_fw_ipi_register(vpu->ctx->dev->fw_handler,
						 vpu->core_id, vpu->handler,
						 "vdec", NULL);
		if (err) {
			mtk_vcodec_err(vpu, "vpu_ipi_register core fail status=%d", err);
			return err;
		}
	}

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = AP_IPIMSG_DEC_INIT;
	msg.ap_inst_addr = (unsigned long)vpu;
	msg.codec_type = vpu->codec_type;

	mtk_vcodec_debug(vpu, "vdec_inst=%p", vpu);

	err = vcodec_vpu_send_msg(vpu, (void *)&msg, sizeof(msg));

	if (IS_ERR_OR_NULL(vpu->vsi)) {
		mtk_vcodec_err(vpu, "invalid vdec vsi, status=%d", err);
		return -EINVAL;
	}

	mtk_vcodec_debug(vpu, "- ret=%d", err);
	return err;
}

int vpu_dec_start(struct vdec_vpu_inst *vpu, uint32_t *data, unsigned int len)
{
	struct vdec_ap_ipi_dec_start msg;
	int i;
	int err = 0;

	mtk_vcodec_debug_enter(vpu);

	if (len > ARRAY_SIZE(msg.data)) {
		mtk_vcodec_err(vpu, "invalid len = %d\n", len);
		return -EINVAL;
	}

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = AP_IPIMSG_DEC_START;
	if (vpu->fw_abi_version < 2)
		msg.vpu_inst_addr = vpu->inst_addr;
	else
		msg.inst_id = vpu->inst_id;

	for (i = 0; i < len; i++)
		msg.data[i] = data[i];
	msg.codec_type = vpu->codec_type;

	err = vcodec_vpu_send_msg(vpu, (void *)&msg, sizeof(msg));
	mtk_vcodec_debug(vpu, "- ret=%d", err);
	return err;
}

int vpu_dec_get_param(struct vdec_vpu_inst *vpu, uint32_t *data,
		      unsigned int len, unsigned int param_type)
{
	struct vdec_ap_ipi_get_param msg;
	int err;

	mtk_vcodec_debug_enter(vpu);

	if (len > ARRAY_SIZE(msg.data)) {
		mtk_vcodec_err(vpu, "invalid len = %d\n", len);
		return -EINVAL;
	}

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = AP_IPIMSG_DEC_GET_PARAM;
	msg.inst_id = vpu->inst_id;
	memcpy(msg.data, data, sizeof(unsigned int) * len);
	msg.param_type = param_type;
	msg.codec_type = vpu->codec_type;

	err = vcodec_vpu_send_msg(vpu, (void *)&msg, sizeof(msg));
	mtk_vcodec_debug(vpu, "- ret=%d", err);
	return err;
}

int vpu_dec_core(struct vdec_vpu_inst *vpu)
{
	return vcodec_send_ap_ipi(vpu, AP_IPIMSG_DEC_CORE);
}

int vpu_dec_end(struct vdec_vpu_inst *vpu)
{
	return vcodec_send_ap_ipi(vpu, AP_IPIMSG_DEC_END);
}

int vpu_dec_core_end(struct vdec_vpu_inst *vpu)
{
	return vcodec_send_ap_ipi(vpu, AP_IPIMSG_DEC_CORE_END);
}

int vpu_dec_deinit(struct vdec_vpu_inst *vpu)
{
	return vcodec_send_ap_ipi(vpu, AP_IPIMSG_DEC_DEINIT);
}

int vpu_dec_reset(struct vdec_vpu_inst *vpu)
{
	return vcodec_send_ap_ipi(vpu, AP_IPIMSG_DEC_RESET);
}
