#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelMicroLooper;
extern Model* modelLogisticScratch;
extern Model* modelLFSR8;
extern Model* modelLFSR16;


struct Inlet : app::SvgPort {
	Inlet() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/Inlet.svg")));
	}
};

struct Outlet : app::SvgPort {
	Outlet() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/Outlet.svg")));
	}
};

struct TsKnob : app::SvgKnob {
	widget::SvgWidget* bg;

	TsKnob() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

        bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		speed = 2.f;
	}
};

struct TsKnobStd : TsKnob {
    TsKnobStd() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobStd.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobStd_bg.svg")));
    }
};

struct TsKnobBig : TsKnob {
    TsKnobBig() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobBig.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobBig_bg.svg")));
    }
};

struct TsKnobLarge : TsKnob {
    TsKnobLarge() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobLarge.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/KnobLarge_bg.svg")));
    }
};

struct TsButton : app::SvgSwitch {
	TsButton() {
		shadow->opacity = 0.02;
	}
};

struct TsButtonLarge : TsButton {
	TsButtonLarge() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/BtnBig_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/BtnBig_1_red.svg")));
	}
};

struct TsButtonLargePush : TsButtonLarge {
	TsButtonLargePush() {
		momentary = false;
	}
};

struct TsButtonStd : TsButton {
	TsButtonStd() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/BtnStd_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/BtnStd_1_red.svg")));
	}
};

struct TsButtonStdPush : TsButtonStd {
	TsButtonStdPush() {
		momentary = false;
	}
};

template <typename TBase>
struct TsLightStd : TSvgLight<TBase> {
	TsLightStd() {
		this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/LightStd.svg")));
	}
};

template <typename TBase>
struct TsLightSquareLarge : RectangleLight<TSvgLight<TBase>> {
	TsLightSquareLarge() {
		this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/LightSquareLarge.svg")));
	}
};