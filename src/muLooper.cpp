#include "plugin.hpp"

struct MuLooper : Module {
	enum ParamId {
		REC_PARAM,
		SPLT_PARAM,
		SCN_PARAM,
		SCN_MOD_PARAM,
		SPD_PARAM,
		SPD_MOD_PARAM,
		FBK_PARAM,
		FBK_MOD_PARAM,
		WET_PARAM,
		WET_MOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		IN_INPUT,
		REC_INPUT,
		SPLT_INPUT,
		SCN_INPUT,
		FBK_INPUT,
		SPD_INPUT,
		WET_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		MONO_OUTPUT,
		POLY_OUTPUT,
		EOC_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	const static int MAX_CHUNKS = 16;
	const static int MAX_BITS = 16;
	const static int MAX_LENGTH = 1 << MAX_BITS;

	MuLooper() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(REC_PARAM, 0.f, 1.f, 0.f, "Recording");
		configParam(SPLT_PARAM, 1.0, MAX_CHUNKS, 1.0, "Split", "");
		configParam(SCN_PARAM, 0.f, 1.f, 0.f, "Scan", "");
		configParam(SCN_MOD_PARAM, -1.f, 1.f, 0.f, "Scan modulation amount", "");
		configParam(SPD_PARAM, -4.f, 4.f, 1.f, "Speed", " samples");
		configParam(SPD_MOD_PARAM, -1.f, 1.f, 0.f, "Speed modulation amount", "");
		configParam(FBK_PARAM, 0.f, 1.f, 0.f, "Feedback", "", 0.f, 100.f, 0.f);
		configParam(FBK_MOD_PARAM, -1.f, 1.f, 0.f, "Feedback modulation amount", "");
		configParam(WET_PARAM, 0.f, 1.f, 1.f, "Dry/Wet", "", 0.f, 100.f, 0.f);
		configParam(WET_MOD_PARAM, -1.f, 1.f, 0.f, "Dry/Wet modulation amount", "");
		configInput(IN_INPUT, "Signal");
		configInput(REC_INPUT, "Record");
		configInput(SPLT_INPUT, "Split");
		configInput(SCN_INPUT, "Scan modulation");
		configInput(SPD_INPUT, "Speed modulation");
		configInput(FBK_INPUT, "Feedback modulation");
		configInput(WET_INPUT, "Dry/Wet modulatiob");
		configOutput(MONO_OUTPUT, "Mono");
		configOutput(POLY_OUTPUT, "Poly");
		configOutput(EOC_OUTPUT, "End-Of-The-Record-Cycle");

		paramQuantities[SPLT_PARAM]->snapEnabled = true;
	}

	float currentSamples[MAX_CHUNKS] = {0.f};
	bool isRecord = false;
	bool isRecorded = false;
	float recordBuffer[MAX_LENGTH] = {0.f};
	float playBuffer[MAX_LENGTH] = {0.f};
	int recordPosition = 0;
	float playPosition = 0.f;
	int chunks = -1;
	std::vector<std::string> text = {};
	bool scaleSpeed = false;

	void process(const ProcessArgs& args) override {
		bool doRecording = params[REC_PARAM].getValue() > 0.f;

		if (inputs[REC_INPUT].isConnected()) {
			doRecording |= inputs[REC_INPUT].getVoltage() > 0.f;
		}

		if (!isRecord && doRecording && !isRecorded) {
			recordPosition = 0;
			isRecord = true;
			params[REC_PARAM].setValue(1.0f);
		}

		int chunksNext = (int)params[SPLT_PARAM].getValue();
		if (inputs[SPLT_INPUT].isConnected()) {
			chunksNext = (int)(inputs[SPLT_INPUT].getVoltage() / 10.f * (float)(MAX_CHUNKS - 1)) + 1;
		}
		chunksNext = clamp(chunksNext, 1, MAX_CHUNKS);
		if (chunks < 0) {
			chunks = chunksNext;
		}

		int lengthNext = MAX_LENGTH / chunksNext;
		text = {
			"CHUNKS: " + std::to_string((int)((float)chunksNext * (float)recordPosition / (float)MAX_LENGTH)) + " / "
			+ std::to_string(chunksNext),
			"LENGTH: " + std::to_string(lengthNext)
		};
		int length = MAX_LENGTH / chunks;

		float speed = params[SPD_PARAM].getValue();
		if (inputs[SPD_INPUT].isConnected()) {
			float mod = params[SPD_MOD_PARAM].getValue();
			speed += inputs[SPD_INPUT].getVoltage() / 5.0 * mod;
		}

		float chunkRaw = params[SCN_PARAM].getValue();
		if (inputs[SCN_INPUT].isConnected()) {
			float mod = params[SCN_MOD_PARAM].getValue();
			chunkRaw += inputs[SCN_INPUT].getVoltage() / 10.f * mod;
		}
		chunkRaw = clamp(chunkRaw, 0.0, 1.0);
		chunkRaw *= (float)chunks - 1.0;
		int chunk = (int)chunkRaw;
		float chunkFrac = chunkRaw - (float)chunk;

		float wet = params[WET_PARAM].getValue();
		if (inputs[WET_INPUT].isConnected()) {
			float mod = params[WET_MOD_PARAM].getValue();
			wet += inputs[WET_INPUT].getVoltage() / 10.f * mod;
		}

		float fbk = params[FBK_PARAM].getValue();
		if (inputs[FBK_INPUT].isConnected()) {
			float mod = params[FBK_MOD_PARAM].getValue();
			fbk += inputs[FBK_INPUT].getVoltage() / 10.f * mod;
		}

		bool swapReel = false;
		int pos = (int)playPosition;
		float frac = playPosition - (float)pos;

		float signal = inputs[IN_INPUT].getVoltageSum();
		outputs[POLY_OUTPUT].setChannels(chunks);
		float smoothing = sin(M_PI * playPosition / (float)length);

		for (int i = 0; i < chunks; i++) {
			int realPos = pos + (i + chunk) * length;
			if (realPos >= MAX_LENGTH) realPos -= MAX_LENGTH;
			int realPosNext = realPos + length;
			if (realPosNext >= MAX_LENGTH) realPosNext -= MAX_LENGTH;

			float currentSample0 =
				playBuffer[realPos] * (1 - frac) +
				playBuffer[pos >= length - 1 ? i * length : realPos + 1] * frac;

			float currentSample1 =
				playBuffer[realPosNext] * (1 - frac) +
				playBuffer[pos >= length - 1 ? i * length + length : realPosNext + 1] * frac;

			currentSamples[i] = currentSample0 * (1.f - chunkFrac) + currentSample1 * chunkFrac;
			currentSamples[i] *= smoothing;

			outputs[POLY_OUTPUT].setVoltage(currentSamples[i] * wet +  signal * (1.0f - wet), i);

			if (playPosition < fabs(speed) && isRecorded) {
				swapReel = true;
				isRecorded = false;
			}
		}

		if (swapReel) {
			for (int j = 0; j < MAX_LENGTH; j++) {
				playBuffer[j] = recordBuffer[j];
			}
		}
		if (!isRecord) {
			chunks = chunksNext;
		}

		playPosition += speed / (scaleSpeed ? (float)chunks : 1.f);

		if (playPosition < 0) {
			playPosition += (float)length;
		}
		if (playPosition >= (float)length) {
			playPosition -= (float)length;
		}

		float currentSample = currentSamples[0];
		outputs[MONO_OUTPUT].setVoltage(currentSample * wet + signal * (1.0f - wet));

		bool eoc = false;
		if (isRecord) {
			recordBuffer[recordPosition++] = signal + currentSample * fbk;

			if (recordPosition >= MAX_LENGTH) {
				isRecorded = true;
				isRecord = false;
				params[REC_PARAM].setValue(0.0f);
				eoc = true;
			}
			outputs[EOC_OUTPUT].setVoltage(eoc ? 10.0 : 0.0);
		}
	}

	json_t* dataToJson() override {
		json_t * rootJ = json_object();
		json_object_set_new(rootJ, "scaleSpeed", json_boolean(scaleSpeed));

		json_t * playBufferJ = json_array();
		for (int i = 0; i < MAX_LENGTH; i++) {
			json_array_append_new(playBufferJ, json_real(playBuffer[i]));
		}

		json_object_set_new(rootJ, "playBuffer", playBufferJ);
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t * scaleSpeedJ = json_object_get(rootJ, "scaleSpeed");

		if (scaleSpeedJ) {
			scaleSpeed = json_boolean_value(scaleSpeedJ);
		}

		json_t * playBufferJ = json_object_get(rootJ, "playBuffer");
		if (playBufferJ) {
			size_t i;
			json_t * a;
			json_array_foreach(playBufferJ, i, a) {
				playBuffer[i] = json_real_value(a);
			}
		}
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

		addParam(createParamCentered<FlatButtonStdPush>(mm2px(Vec(13.0, 23.0)), module, MuLooper::REC_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(33.0, 23.0)), module, MuLooper::SPLT_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 63.0)), module, MuLooper::SCN_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(6.0, 63.0)), module, MuLooper::SCN_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(33.0, 63.0)), module, MuLooper::SPD_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(26.0, 63.0)), module, MuLooper::SPD_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 83.0)), module, MuLooper::FBK_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(6.0, 83.0)), module, MuLooper::FBK_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(33.0, 83.0)), module, MuLooper::WET_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(26.0, 83.0)), module, MuLooper::WET_MOD_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 99.0)), module, MuLooper::IN_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 16.0)), module, MuLooper::REC_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 16.0)), module, MuLooper::SPLT_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 56.0)), module, MuLooper::SCN_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 56.0)), module, MuLooper::SPD_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 76.0)), module, MuLooper::FBK_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 76.0)), module, MuLooper::WET_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(35.0, 99.0)), module, MuLooper::MONO_OUTPUT));
		addOutput(createOutputCentered<PolyOutlet>(mm2px(Vec(35.0, 107.0)), module, MuLooper::POLY_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(35.0, 115.0)), module, MuLooper::EOC_OUTPUT));

		FlatDisplay<MuLooper> * display = createWidget<FlatDisplay<MuLooper>>(mm2px(Vec(1.0, 30)));
		display->box.size = mm2px(Vec(39.0, 8.0));
		display->module = module;
		display->fontSize = 10;
		addChild(display);
	}

	void appendContextMenu(Menu* menu) override {
		MuLooper* module = dynamic_cast<MuLooper*>(this->module);
		assert(module);
		menu->addChild(new MenuSeparator);
		menu->addChild(createBoolPtrMenuItem("Scale speed", "", &module->scaleSpeed));
	}
};

Model* modelMuLooper = createModel<MuLooper, MuLooperWidget>("muLooper");