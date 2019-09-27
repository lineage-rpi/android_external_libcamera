/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * control_list.cpp - ControlList tests
 */

#include <iostream>

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>

#include "camera_controls.h"
#include "test.h"

using namespace std;
using namespace libcamera;

class ControlListTest : public Test
{
protected:
	int init()
	{
		cm_ = new CameraManager();

		if (cm_->start()) {
			cout << "Failed to start camera manager" << endl;
			return TestFail;
		}

		camera_ = cm_->get("VIMC Sensor B");
		if (!camera_) {
			cout << "Can not find VIMC camera" << endl;
			return TestSkip;
		}

		return TestPass;
	}

	int run()
	{
		CameraControlValidator validator(camera_.get());
		ControlList list(&validator);

		/* Test that the list is initially empty. */
		if (!list.empty()) {
			cout << "List should to be empty" << endl;
			return TestFail;
		}

		if (list.size() != 0) {
			cout << "List should contain zero items" << endl;
			return TestFail;
		}

		if (list.contains(controls::Brightness)) {
			cout << "List should not contain Brightness control" << endl;
			return TestFail;
		}

		unsigned int count = 0;
		for (auto iter = list.begin(); iter != list.end(); ++iter)
			count++;

		if (count != 0) {
			cout << "List iteration should not produce any item" << endl;
			return TestFail;
		}

		/*
		 * Set a control, and verify that the list now contains it, and
		 * nothing else.
		 */
		list.set(controls::Brightness, 255);

		if (list.empty()) {
			cout << "List should not be empty" << endl;
			return TestFail;
		}

		if (list.size() != 1) {
			cout << "List should contain one item" << endl;
			return TestFail;
		}

		if (!list.contains(controls::Brightness)) {
			cout << "List should contain Brightness control" << endl;
			return TestFail;
		}

		count = 0;
		for (auto iter = list.begin(); iter != list.end(); ++iter)
			count++;

		if (count != 1) {
			cout << "List iteration should produce one item" << endl;
			return TestFail;
		}

		if (list.get(controls::Brightness) != 255) {
			cout << "Incorrest Brightness control value" << endl;
			return TestFail;
		}

		if (list.contains(controls::Contrast)) {
			cout << "List should not contain Contract control" << endl;
			return TestFail;
		}

		/* Update the first control and set a second one. */
		list.set(controls::Brightness, 64);
		list.set(controls::Contrast, 128);

		if (!list.contains(controls::Contrast) ||
		    !list.contains(controls::Contrast)) {
			cout << "List should contain Contrast control" << endl;
			return TestFail;
		}

		if (list.get(controls::Brightness) != 64 ||
		    list.get(controls::Contrast) != 128) {
			cout << "Failed to retrieve control value" << endl;
			return TestFail;
		}

		/*
		 * Update both controls and verify that the container doesn't
		 * grow.
		 */
		list.set(controls::Brightness, 10);
		list.set(controls::Contrast, 20);

		if (list.get(controls::Brightness) != 10 ||
		    list.get(controls::Contrast) != 20) {
			cout << "Failed to update control value" << endl;
			return TestFail;
		}

		if (list.size() != 2) {
			cout << "List should contain two elements" << endl;
			return TestFail;
		}

		/*
		 * Attempt to set an invalid control and verify that the
		 * operation failed.
		 */
		list.set(controls::AwbEnable, true);

		if (list.contains(controls::AwbEnable)) {
			cout << "List shouldn't contain AwbEnable control" << endl;
			return TestFail;
		}

		return TestPass;
	}

	void cleanup()
	{
		if (camera_) {
			camera_->release();
			camera_.reset();
		}

		cm_->stop();
		delete cm_;
	}

private:
	CameraManager *cm_;
	std::shared_ptr<Camera> camera_;
};

TEST_REGISTER(ControlListTest)
