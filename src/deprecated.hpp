#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelMicroLooper;
extern Model* modelLFSR8;
extern Model* modelLFSR16;


struct TsKnob : app::SvgKnob {
	widget::SvgWidget* bg;

	TsKnob() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

        bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		speed = 2.f;
		shadow->opacity = 0.05;
	}
};

struct TsKnobStd : TsKnob {
    TsKnobStd() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/deprecated/KnobStd.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/deprecated/KnobStd_bg.svg")));
    }
};

struct TsKnobBig : TsKnob {
    TsKnobBig() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/deprecated/KnobBig.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/deprecated/KnobBig_bg.svg")));
    }
};

struct TsKnobLarge : TsKnob {
    TsKnobLarge() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/deprecated/KnobLarge.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/deprecated/KnobLarge_bg.svg")));
    }
};

struct TsButton : app::SvgSwitch {
	TsButton() {
		shadow->opacity = 0.05;
	}
};

struct TsButtonLarge : TsButton {
	TsButtonLarge() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/deprecated/BtnBig_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/deprecated/BtnBig_1_red.svg")));
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
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/deprecated/BtnStd_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/deprecated/BtnStd_1_red.svg")));
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
		this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/deprecated/LightStd.svg")));
	}
};

template <typename TBase>
struct TsLightSquareLarge : RectangleLight<TSvgLight<TBase>> {
	TsLightSquareLarge() {
		this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/deprecated/LightSquareLarge.svg")));
	}
};