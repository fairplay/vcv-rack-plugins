#include "plugin.hpp"
#include "KarplusStrong.cpp"

struct Pluck : Module {
	enum ParamId {
		PLUCK_PARAM,
		FREQ_PARAM,
		DAMP_PARAM,
		DAMP_MOD_PARAM,
		ANGLE_PARAM,
		ANGLE_MOD_PARAM,
		DECAY_PARAM,
		DECAY_MOD_PARAM,
		POSITION_PARAM,
		POSITION_MOD_PARAM,
		LEVEL_PARAM,
		LEVEL_MOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		PLUCK_INPUT,
		FREQ_INPUT,
		DAMP_INPUT,
		ANGLE_INPUT,
		DECAY_INPUT,
		POSITION_INPUT,
		LEVEL_INPUT,
		EXCITER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	KarplusStrong * kss[PORT_MAX_CHANNELS];
	Exciter * exc = new Exciter();
	dsp::SchmittTrigger trigger;

	Pluck() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PLUCK_PARAM, 0.f, 1.f, 0.f, "Pluck", "");
		configParam(FREQ_PARAM, -30.f, 30.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(DAMP_PARAM, 0.f, 1.f, 0.5f, "Damping factor", "");
		configParam(DAMP_MOD_PARAM, -1.f, 1.f, 0.f, "Damping factor modulation amount", "");
		configParam(ANGLE_PARAM, 0.f, 1.f, 0.f, "Pluck angle", "");
		configParam(ANGLE_MOD_PARAM, -1.f, 1.f, 0.f, "Angle modulation amount", "");
		configParam(DECAY_PARAM, 0.1f, 10.f, 4.f, "Decay", " seconds");
		configParam(DECAY_MOD_PARAM, -1.f, 1.f, 0.f, "Decay modulation amount", "");
		configParam(POSITION_PARAM, 0.f, 1.f, 0.5f, "Pluck position", "");
		configParam(POSITION_MOD_PARAM, -1.f, 1.f, 0.f, "Pluck position modulation amount", "");
		configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Pluck level", "");
		configParam(LEVEL_MOD_PARAM, -1.f, 1.f, 0.f, "Pluck level modulation amount", "");

		configInput(PLUCK_INPUT, "Pluck");
		configInput(FREQ_INPUT, "Frequency");
		configInput(DAMP_INPUT, "Damping factor modulation");
		configInput(ANGLE_INPUT, "Pluck angle modulation");
		configInput(DECAY_INPUT, "Decay modulation");
		configInput(POSITION_INPUT, "Pluck position modulation");
		configInput(LEVEL_INPUT, "Pluck level modulation");
		configInput(EXCITER_INPUT, "Exciter");

		configOutput(OUT_OUTPUT);

		for(int i = 0; i < PORT_MAX_CHANNELS; i++) {
			kss[i] = new KarplusStrong();
		}
	}

	void process(const ProcessArgs& args) override {
		bool triggered = trigger.process(inputs[PLUCK_INPUT].getVoltage(), 0.1f, 2.f);
		triggered &= trigger.isHigh();
		triggered |= params[PLUCK_PARAM].getValue() > 0.0;
		if (triggered) {
			params[PLUCK_PARAM].setValue(0.0);
		}

		float damp = params[DAMP_PARAM].getValue();
		if (inputs[DAMP_INPUT].isConnected()) {
			damp += params[DAMP_MOD_PARAM].getValue() * inputs[DAMP_INPUT].getVoltage() / 10.f;
		}
		damp = clamp(damp, 0.0, 1.0);

		float decay = params[DECAY_PARAM].getValue();
		if (inputs[DECAY_INPUT].isConnected()) {
			decay += params[DECAY_MOD_PARAM].getValue() * inputs[DECAY_INPUT].getVoltage();
		}
		decay = clamp(decay, 0.1, 100.0);

		float pluckAngle = params[ANGLE_PARAM].getValue();
		if (inputs[ANGLE_INPUT].isConnected()) {
			pluckAngle += params[ANGLE_MOD_PARAM].getValue() * inputs[ANGLE_INPUT].getVoltage() / 10.f;
		}
		pluckAngle = clamp(pluckAngle, 0.0, 0.9);

		float pluckPosition = params[POSITION_PARAM].getValue();
		if (inputs[POSITION_INPUT].isConnected()) {
			pluckPosition += params[POSITION_MOD_PARAM].getValue() * inputs[POSITION_INPUT].getVoltage() / 10.f;
		}
		pluckPosition = clamp(pluckPosition, 0.0, 1.0);

		float level = params[LEVEL_PARAM].getValue();
		if (inputs[LEVEL_INPUT].isConnected()) {
			level += params[LEVEL_MOD_PARAM].getValue() * inputs[LEVEL_INPUT].getVoltage() / 10.f;
		}
		level = clamp(level, 0.0, 1.0);

		int ch = inputs[FREQ_INPUT].getChannels();
		outputs[OUT_OUTPUT].setChannels(ch);
		float freqVoltage0 = params[FREQ_PARAM].getValue() / 12.0;

		for (int i = 0; i <= ch; i++) {
			KarplusStrong * ks = kss[i];
			ks->sampleRate = args.sampleRate;
			float freqVoltage = freqVoltage0 + inputs[FREQ_INPUT].getVoltage(i);
			ks->setFreq(dsp::FREQ_C4 * dsp::approxExp2_taylor5(freqVoltage + 30.f) / std::pow(2.f, 30.f));
			ks->setDecayFromT60(decay);
			ks->damp = damp;
			ks->pluckAngle = pluckAngle;
			ks->pluckPosition = pluckPosition;
			ks->pluckLevel = level;

			if (triggered) {
				ks->pluck(exc);
			}
			outputs[OUT_OUTPUT].setVoltage(ks->next(), i);
		}

		float excVoltage = inputs[EXCITER_INPUT].getVoltageSum();
		exc->excite(inputs[EXCITER_INPUT].isConnected(), excVoltage);
	}

	void onReset(const ResetEvent& e) override {
	}
};


struct PluckWidget : ModuleWidget {
	PluckWidget(Pluck* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Pluck.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<FlatButtonStdLatch>(mm2px(Vec(13.0, 23.0)), module, Pluck::PLUCK_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(33.0, 23.0)), module, Pluck::FREQ_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 43.0)), module, Pluck::DAMP_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(4.5, 43.0)), module, Pluck::DAMP_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(33.0, 43.0)), module, Pluck::ANGLE_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(24.5, 43.0)), module, Pluck::ANGLE_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 63.0)), module, Pluck::DECAY_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(4.5, 63.0)), module, Pluck::DECAY_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(33.0, 63.0)), module, Pluck::POSITION_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(24.5, 63.0)), module, Pluck::POSITION_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 83.0)), module, Pluck::LEVEL_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(4.5, 83.0)), module, Pluck::LEVEL_MOD_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 16.0)), module, Pluck::PLUCK_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 16.0)), module, Pluck::FREQ_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 36.0)), module, Pluck::DAMP_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 36.0)), module, Pluck::ANGLE_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 56.0)), module, Pluck::DECAY_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 56.0)), module, Pluck::POSITION_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 76.0)), module, Pluck::LEVEL_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 99.0)), module, Pluck::EXCITER_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(35.0, 99.0)), module, Pluck::OUT_OUTPUT));
	}
};


Model* modelPluck = createModel<Pluck, PluckWidget>("Pluck");