#include "plugin.hpp"


struct MuLooper : Module {
	enum ParamId {
		REC_PARAM,
		LEN_PARAM,
		SCN_PARAM,
		SPD_PARAM,
		FBK_PARAM,
		WET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		IN_INPUT,
		REC_INPUT,
		LEN_INPUT,
		SCN_INPUT,
		FBK_INPUT,
		SPD_INPUT,
		WET_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		MONO_OUTPUT,
		POLY_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		REC_LED_0_LIGHT,
		REC_LED_1_LIGHT,
		REC_LED_2_LIGHT,
		REC_LED_3_LIGHT,
		REC_LED_4_LIGHT,
		REC_LED_5_LIGHT,
		REC_LED_6_LIGHT,
		REC_LED_7_LIGHT,
		CH_LED_0_LIGHT,
		CH_LED_1_LIGHT,
		CH_LED_2_LIGHT,
		CH_LED_3_LIGHT,
		CH_LED_4_LIGHT,
		CH_LED_5_LIGHT,
		CH_LED_6_LIGHT,
		CH_LED_7_LIGHT,
		CH_LED_8_LIGHT,
		CH_LED_9_LIGHT,
		CH_LED_10_LIGHT,
		CH_LED_11_LIGHT,
		CH_LED_12_LIGHT,
		CH_LED_13_LIGHT,
		CH_LED_14_LIGHT,
		CH_LED_15_LIGHT,
		LIGHTS_LEN
	};

	const static int MIN_BITS = 12;
	const static int MAX_BITS = 16;
	const static int MAX_LENGTH = 1 << MAX_BITS;
	const static int MAX_CHUNKS = (1 << (MAX_BITS - MIN_BITS)) - 1;

	MuLooper() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(REC_PARAM, 0.f, 1.f, 0.f, "Recording");
		configParam(LEN_PARAM, (float)MIN_BITS, (float)MAX_BITS, (float)MAX_BITS, "Length", "", 2.f, 1.f, 0.f);
		configParam(SCN_PARAM, 0.f, 1.f, 0.f, "Chunk", "");
		configParam(SPD_PARAM, -5.f, 5.f, 1.f, "Speed", " samples");
		configParam(FBK_PARAM, 0.f, 1.f, 0.f, "Feedback", "", 0.f, 100.f, 0.f);
		configParam(WET_PARAM, 0.f, 1.f, 1.f, "Dry/Wet", "", 0.f, 100.f, 0.f);
		configInput(IN_INPUT, "Signal");
		configInput(REC_INPUT, "Record");
		configInput(LEN_INPUT, "Record");
		configInput(SCN_INPUT, "Scan position / Channel");
		configInput(FBK_INPUT, "Feedback");
		configInput(SPD_INPUT, "Speed");
		configInput(WET_INPUT, "Dry/Wet");
		configOutput(MONO_OUTPUT, "Mono");
		configOutput(POLY_OUTPUT, "Poly");

		paramQuantities[LEN_PARAM]->snapEnabled = true;
	}

	bool isRecord = false;
	float recordBuffer[MAX_LENGTH] = {0.f};
	float playBuffer[MAX_LENGTH] = {0.f};
	int recordPosition = 0;
	float playPosition = 0.f;
	int length0 = MAX_LENGTH;

	void process(const ProcessArgs& args) override {
		bool doRecording = params[REC_PARAM].getValue() > 0.f;

		if (inputs[REC_INPUT].isConnected()) {
			doRecording = inputs[REC_INPUT].getVoltage() > 0.f;
		}

		if (isRecord != doRecording && doRecording) {
			recordPosition = 0;
			isRecord = true;
			params[REC_PARAM].setValue(1.0f);
		}


		float speed = params[SPD_PARAM].getValue();
		if (inputs[SPD_INPUT].isConnected()) {
			speed = (speed + 5.f) * (inputs[SPD_INPUT].getVoltage() - 5.0f) / 10.f;
		}

		int lengthRaw = (int)params[LEN_PARAM].getValue();

		if (inputs[LEN_INPUT].isConnected()) {
			lengthRaw = (int)(inputs[LEN_INPUT].getVoltage() / 10.f * ((float)MAX_BITS - (float)MIN_BITS) + (float)MIN_BITS);
		}

		int length = 1 << (lengthRaw - 1);
		if (length0 != length) {
			playPosition = 0;
		}
		length0 = length;

		int channels = PORT_MAX_CHANNELS >> (lengthRaw - MIN_BITS);
		outputs[POLY_OUTPUT].setChannels(channels);
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			lights[CH_LED_0_LIGHT + i].setBrightness(
				i >= channels ? 0.1f : 1.0f
			);
		}

		float chunkRaw = params[SCN_PARAM].getValue();
		if (inputs[SCN_INPUT].isConnected()) {
			chunkRaw *= inputs[SCN_INPUT].getVoltage() / 10.f;
		}
		chunkRaw = clamp(chunkRaw, 0.f, 1.0f);
		chunkRaw *= ((float)MAX_LENGTH / (float)length - 1.f);
		int chunk = (int)chunkRaw;
		float chunkFrac = chunkRaw - (float)chunk;

		float wet = params[WET_PARAM].getValue();
		if (inputs[WET_INPUT].isConnected()) {
			wet *= inputs[WET_INPUT].getVoltage() / 10.f;
		}

		float fbk = params[FBK_PARAM].getValue();
		if (inputs[FBK_INPUT].isConnected()) {
			fbk *= inputs[FBK_INPUT].getVoltage() / 10.f;
		}

		float currentSamples[PORT_MAX_CHANNELS] = {0.f};

		int pos = (int)playPosition;
		float frac = playPosition - (float)pos;

		for (int i = 0; i < channels; i++) {
			int realPos = pos + i * length;
			int realPosNext = realPos + length;
			if (realPosNext >= MAX_LENGTH) realPosNext -= MAX_LENGTH;

			float currentSample0 =
				playBuffer[realPos] * (1 - frac) +
				playBuffer[pos >= length - 1 ? i * length : realPos + 1] * frac;

			float currentSample1 =
				playBuffer[realPosNext] * (1 - frac) +
				playBuffer[pos >= length - 1 ? i * length + length : realPosNext + 1] * frac;

			currentSamples[i] = currentSample0 * (1.f - chunkFrac) + currentSample1 * chunkFrac;

			playPosition += speed;
		}

		float currentSample = currentSamples[chunk];

		if (playPosition < 0) {
			playPosition += (float)length;
		}
		if (playPosition >= (float)length) {
			playPosition -= (float)length;
		}

		float signal = inputs[IN_INPUT].getVoltage();
		if (isRecord) {
			recordBuffer[recordPosition++] = signal + currentSample * fbk;

			float lightNumber = (float)(REC_LED_7_LIGHT - REC_LED_0_LIGHT + 1) * (float)recordPosition / (float)MAX_LENGTH;
			lights[REC_LED_0_LIGHT + lightNumber].setBrightness(lightNumber - (int)lightNumber);

			if (recordPosition >= MAX_LENGTH) {
				for (int i = 0; i < MAX_LENGTH; i++) {
					playBuffer[i] = recordBuffer[i];
				}
				isRecord = false;
				recordPosition = 0;
				params[REC_PARAM].setValue(0.0f);
			}
		}

		for (int i = 0; i < channels; i++) {
			outputs[POLY_OUTPUT].setVoltage(currentSamples[i] * wet +  signal * (1.0f - wet), i);
		}

		outputs[MONO_OUTPUT].setVoltage(currentSample * wet + signal * (1.0f - wet));
	}
};

struct MuLooperWidget : ModuleWidget {
	MuLooperWidget(MuLooper* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/muLooper.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<FlatButtonStdPush>(mm2px(Vec(14.0, 23.0)), module, MuLooper::REC_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(14.0, 43.0)), module, MuLooper::LEN_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(14.0, 63.0)), module, MuLooper::SCN_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(33.0, 63.0)), module, MuLooper::SPD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(14.0, 83.0)), module, MuLooper::FBK_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(33.0, 83.0)), module, MuLooper::WET_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(7.0, 99.0)), module, MuLooper::IN_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(7.0, 16.0)), module, MuLooper::REC_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(7.0, 36.0)), module, MuLooper::LEN_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(7.0, 56.0)), module, MuLooper::SCN_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(7.0, 76.0)), module, MuLooper::FBK_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 56.0)), module, MuLooper::SPD_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 76.0)), module, MuLooper::WET_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(21.0, 99.0)), module, MuLooper::MONO_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(35.0, 99.0)), module, MuLooper::POLY_OUTPUT));

		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(21.0, 23.0)), module, MuLooper::REC_LED_0_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(23.0, 23.0)), module, MuLooper::REC_LED_1_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(25.0, 23.0)), module, MuLooper::REC_LED_2_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(27.0, 23.0)), module, MuLooper::REC_LED_3_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(29.0, 23.0)), module, MuLooper::REC_LED_4_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(31.0, 23.0)), module, MuLooper::REC_LED_5_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(33.0, 23.0)), module, MuLooper::REC_LED_6_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(35.0, 23.0)), module, MuLooper::REC_LED_7_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(29.0, 40.0)), module, MuLooper::CH_LED_0_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(31.0, 40.0)), module, MuLooper::CH_LED_1_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(33.0, 40.0)), module, MuLooper::CH_LED_2_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(35.0, 40.0)), module, MuLooper::CH_LED_3_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(29.0, 42.0)), module, MuLooper::CH_LED_4_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(31.0, 42.0)), module, MuLooper::CH_LED_5_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(33.0, 42.0)), module, MuLooper::CH_LED_6_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(35.0, 42.0)), module, MuLooper::CH_LED_7_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(29.0, 44.0)), module, MuLooper::CH_LED_8_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(31.0, 44.0)), module, MuLooper::CH_LED_9_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(33.0, 44.0)), module, MuLooper::CH_LED_10_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(35.0, 44.0)), module, MuLooper::CH_LED_11_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(29.0, 46.0)), module, MuLooper::CH_LED_12_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(31.0, 46.0)), module, MuLooper::CH_LED_13_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(33.0, 46.0)), module, MuLooper::CH_LED_14_LIGHT));
		addChild(createLightCentered<FlatLightSquare<GrayIndicatorLight>>(mm2px(Vec(35.0, 46.0)), module, MuLooper::CH_LED_15_LIGHT));
	}
};


Model* modelMuLooper = createModel<MuLooper, MuLooperWidget>("muLooper");