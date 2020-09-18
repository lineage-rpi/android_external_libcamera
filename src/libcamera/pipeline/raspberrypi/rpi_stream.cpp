/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * rpi_stream.cpp - Raspberry Pi device stream abstraction class.
 */
#include "rpi_stream.h"

#include "libcamera/internal/log.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(RPISTREAM)

namespace RPi {

V4L2VideoDevice *RPiStream::dev() const
{
	return dev_.get();
}

std::string RPiStream::name() const
{
	return name_;
}

void RPiStream::reset()
{
	external_ = false;
	clearBuffers();
}

void RPiStream::setExternal(bool external)
{
	/* Import streams cannot be external. */
	ASSERT(!external || !importOnly_);
	external_ = external;
}

bool RPiStream::isExternal() const
{
	return external_;
}

void RPiStream::setExportedBuffers(std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	std::transform(buffers->begin(), buffers->end(), std::back_inserter(bufferList_),
		       [](std::unique_ptr<FrameBuffer> &b) { return b.get(); });
}

const std::vector<FrameBuffer *> &RPiStream::getBuffers() const
{
	return bufferList_;
}

bool RPiStream::findFrameBuffer(FrameBuffer *buffer) const
{
	if (importOnly_)
		return false;

	if (std::find(bufferList_.begin(), bufferList_.end(), buffer) != bufferList_.end())
		return true;

	return false;
}

int RPiStream::prepareBuffers(unsigned int count)
{
	int ret;

	if (!importOnly_) {
		if (count) {
			/* Export some frame buffers for internal use. */
			ret = dev_->exportBuffers(count, &internalBuffers_);
			if (ret < 0)
				return ret;

			/* Add these exported buffers to the internal/external buffer list. */
			setExportedBuffers(&internalBuffers_);

			/* Add these buffers to the queue of internal usable buffers. */
			for (auto const &buffer : internalBuffers_)
				availableBuffers_.push(buffer.get());
		}

		/* We must import all internal/external exported buffers. */
		count = bufferList_.size();
	}

	return dev_->importBuffers(count);
}

int RPiStream::queueBuffer(FrameBuffer *buffer)
{
	/*
	 * A nullptr buffer implies an external stream, but no external
	 * buffer has been supplied. So, pick one from the availableBuffers_
	 * queue.
	 */
	if (!buffer) {
		if (availableBuffers_.empty()) {
			LOG(RPISTREAM, Warning) << "No buffers available for "
						<< name_;
			return -EINVAL;
		}

		buffer = availableBuffers_.front();
		availableBuffers_.pop();
	}

	LOG(RPISTREAM, Debug) << "Queuing buffer " << buffer->cookie()
			      << " for " << name_;

	int ret = dev_->queueBuffer(buffer);
	if (ret) {
		LOG(RPISTREAM, Error) << "Failed to queue buffer for "
				      << name_;
	}

	return ret;
}

void RPiStream::returnBuffer(FrameBuffer *buffer)
{
	/* This can only be called for external streams. */
	assert(external_);

	availableBuffers_.push(buffer);
}

int RPiStream::queueAllBuffers()
{
	int ret;

	if (external_)
		return 0;

	while (!availableBuffers_.empty()) {
		ret = queueBuffer(availableBuffers_.front());
		if (ret < 0)
			return ret;

		availableBuffers_.pop();
	}

	return 0;
}

void RPiStream::releaseBuffers()
{
	dev_->releaseBuffers();
	clearBuffers();
}

void RPiStream::clearBuffers()
{
	availableBuffers_ = std::queue<FrameBuffer *>{};
	internalBuffers_.clear();
	bufferList_.clear();
}

} /* namespace RPi */

} /* namespace libcamera */
