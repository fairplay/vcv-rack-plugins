#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelLogisticScratch;
extern Model* modelLFSR8Poly;
extern Model* modelLFSR16Poly;
extern Model* modelDroplets;
extern Model* modelChaosMaps;
extern Model* modelMuLooper;
extern Model* modelPluck;


struct Inlet : app::SvgPort {
	Inlet() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/Inlet.svg")));
		shadow->opacity = 0.0;
	}
};

struct Outlet : app::SvgPort {
	Outlet() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/Outlet.svg")));
		shadow->opacity = 0.0;
	}
};

struct PolyOutlet : app::SvgPort {
	PolyOutlet() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/PolyOutlet.svg")));
		shadow->opacity = 0.0;
	}
};

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

struct FlatKnobMod : FlatKnob {
    FlatKnobMod() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatCtrlKnobStd.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatCtrlKnobStd_bg.svg")));
    }
};

struct FlatKnobStd : FlatKnob {
    FlatKnobStd() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatKnobStd.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatKnobStd_bg.svg")));
    }
};

struct FlatKnobSmall : FlatKnob {
    FlatKnobSmall() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatKnobSmall.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatKnobSmall_bg.svg")));
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
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatBtnStd_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatBtnStd_1.svg")));
	}
};

struct FlatButtonStdLatch : FlatButtonStd {
	FlatButtonStdLatch() {
		momentary = false;
		latch = true;
	}
};

struct FlatButtonStdPush : FlatButtonStd {
	FlatButtonStdPush() {
		momentary = false;
	}
};

struct FlatButtonTiny : FlatButton {
	FlatButtonTiny() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatBtnTiny_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatBtnTiny_1.svg")));
	}
};

struct FlatButtonTinyPush : FlatButtonTiny {
	FlatButtonTinyPush() {
		momentary = false;
	}
};

struct FlatSwitch : FlatButton {
	FlatSwitch() {
		momentary = false;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatSwitch_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/flat/FlatSwitch_1.svg")));
	}
};

template <typename TBase = ModuleLightWidget>
struct TBlackWhiteLight : TBase {
	TBlackWhiteLight() {
		this->borderColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
		this->bgColor = SCHEME_BLACK;
		this->addBaseColor(SCHEME_WHITE);
	}
};
using BlackWhiteLight = TBlackWhiteLight<>;

template <typename TBase>
struct FlatLight : RectangleLight<TBase> {
	void drawLayer(const widget::Widget::DrawArgs& args, int layer) override {
		if (layer == 1) {
			// Use the formula `lightColor * (1 - dest) + dest` for blending
			nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
			this->drawLight(args);
		}

		Widget::drawLayer(args, layer);
	}
};

template <typename TBase>
struct FlatLightSquare : FlatLight<TBase> {
	FlatLightSquare() {
		this->box.size = mm2px(math::Vec(1.5f, 1.5f));
	}
};

template <typename TBase>
struct FlatLightSquareStd : FlatLight<TBase> {
	FlatLightSquareStd() {
		this->box.size = mm2px(math::Vec(3.5f, 3.5f));
	}
};

template <class TModule>
struct FlatDisplay : widget::Widget {
	TModule * module;
	std::vector<std::string> text = {};
	std::shared_ptr<Font> font;
	int fontSize = 9;

	FlatDisplay() {
		std::string fontPath = asset::plugin(pluginInstance, "res/fonts/MonoBold.ttf");
		font = APP->window->loadFont(fontPath);
	}

	void step() override {
		if (!module) {
			return;
		}
		text = module->text;
	}

	void draw(const DrawArgs& args) override {
		if (!font) {
			return;
		}

		nvgSave(args.vg);
		math::Rect r = box.zeroPos().shrink(mm2px(math::Vec(1, 0)));;
		nvgBeginPath(args.vg);
		nvgRect(args.vg, RECT_ARGS(r));
		nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x80));
		nvgStrokeWidth(args.vg, 0.5);
		nvgStroke(args.vg);

		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, 0.0);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, SCHEME_WHITE);
		nvgFontSize(args.vg, fontSize);

		int i = 0;
		for (std::string str : text) {
			nvgText(args.vg, mm2px(2.0), mm2px(2.0) + i * (fontSize + 2), str.c_str(), NULL);
			i += 1;
		}
		nvgRestore(args.vg);
	}
};