#include "plugin.hpp"

struct Exciter  {
	std::vector<float> x;
	int xPos = 0;
	int maxLength = 4096;

    Exciter() {
        x.reserve(maxLength);
 		for(int i = 0; i < maxLength; i++) {
            x[i] = 0.0;
        }
    }

	void excite(bool excInput, float excVoltage) {
		float excIn = 0.0;
        if (excInput) {
            excIn = excVoltage * 0.5;
        } else {
            excIn = rand() / (RAND_MAX + 1.0) - 0.5;
        }
        x[xPos] = excIn;
		xPos = (xPos + 1) % maxLength;
	}
};

struct KarplusStrong {
	std::vector<float> y;
	int maxLength = 4096;
	float lengthRaw = 1.0;
	int length = 1;
	int sampleRate;
	int yPos = 0;

    float freq = dsp::FREQ_C4;
    float pluckAngle = 0.0;
    float pluckPosition = 0.5;
    float pluckLevel = 1.0;
	float damp = 0.8;
	float t60 = 4.0;
    float decay = 0.0;

    float sample2 = 0.0;
    float sample1 = 0.0;

	KarplusStrong() {
		y.reserve(maxLength);
 		for(int i = 0; i < maxLength; i++) {
            y[i] = 0.0;
        }
	}

	void pluck(Exciter * exc) {
        float pluckLengthRaw = 1.0;
        if (pluckPosition >= 1.0 / lengthRaw) {
            pluckLengthRaw = pluckPosition * lengthRaw;
        }
        int pluckLength = (int)pluckLengthRaw;
        float pluckFrac = pluckLengthRaw - (float)pluckLength;

        float L = pluckLevel;
        float L0 = pow(L, 1.0 / 3.0);
        float Lw = M_PI / lengthRaw;
        float Lgain = Lw / (1.0 + Lw);
        float Lpole2 = (1.0 - Lw) / (1.0 + Lw);

		float prevForAngleFilter = 0.0;
        float prevForLevelFilter = 0.0;

		for(int i = 0; i < length; i++) {
            y[i] = exc->x[i];
            y[i] = y[i] * (1.0 - pluckAngle) + prevForAngleFilter * pluckAngle;
			prevForAngleFilter = y[i];

            float y0 = 0.0;
            if (i >= pluckLength) {
                y0 = y[i - pluckLength] * (1.0 - pluckFrac) + y[i - pluckLength + 1] * pluckFrac;
            }
            y[i] -= y0;

            float lp2out = y[i] * Lgain + prevForLevelFilter * Lpole2;
            y[i] = L * L0 * y[i] + (1.0 - L) * lp2out;
            prevForLevelFilter = y[i];
		}
	}

    void setFreq(float freq) {
		lengthRaw = fmin(sampleRate / freq, (float)maxLength);
        if (lengthRaw < 1.0) lengthRaw = 1.0;
		length = (int)lengthRaw;
    }

    void setDecayFromT60(float t60) {
		decay = pow(0.001, 1.0 / (freq * t60));
    }

	float next() {
        float frac = lengthRaw - (float)length;
		int yPos1 = (yPos + 1) % length;

        float h0 = (2.0 - damp) / 2.0;
        float h1 = damp / 4.0;

		float currentSample = y[yPos] * (1.0 - frac) + y[yPos1] * frac;
		y[yPos] = decay * (h0 * sample1 + h1 * (currentSample + sample2));

        sample2 = sample1;
        sample1 = currentSample;
		yPos = yPos1;

		return currentSample;
	}
};