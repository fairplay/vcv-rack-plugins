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
extern Model* modelDroplets;
extern Model* modelChaos;


struct Inlet : app::SvgPort {
	Inlet() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/Inlet.svg")));
		shadow->opacity = 0.05;
	}
};

struct Outlet : app::SvgPort {
	Outlet() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/Outlet.svg")));
		shadow->opacity = 0.05;
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
		shadow->opacity = 0.05;
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
		shadow->opacity = 0.05;
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
struct TsLightSquareLarge : RectangleLight<TBase> {
	TsLightSquareLarge() {
		this->bgColor = SCHEME_BLACK;
		this->box.size = mm2px(math::Vec(10.f, 10.f));
	}
};

template <typename TBase>
struct TsLightSquareRect : RectangleLight<TBase> {
	TsLightSquareRect() {
		this->bgColor = SCHEME_BLACK;
		this->box.size = mm2px(math::Vec(6.f, 10.f));
	}
};

/* Flat Widgets */

struct FlatKnob : app::SvgKnob {
	widget::SvgWidget* bg;

	FlatKnob() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

        bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		speed = 2.f;
		shadow->opacity = 0.f;
	}
};

struct FlatKnobStd : FlatKnob {
    FlatKnobStd() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/FlatKnobStd.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/FlatKnobStd_bg.svg")));
    }
};

struct FlatButton : app::SvgSwitch {
	FlatButton() {
		shadow->opacity = 0.f;
	}
};

struct FlatButtonStd : FlatButton {
	FlatButtonStd() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/FlatBtnStd_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/FlatBtnStd_1.svg")));
	}
};
