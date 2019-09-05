#include <MIDI.h>        // access to serial (5 pin DIN) MIDI
#include <USBHost_t36.h> // access to USB MIDI devices (plugged into 2nd USB port)

#define PIN_LED 13

// create Serial MIDI port
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, dinMidi);

// create the ports for Push's USB plugged into Teensy's 2nd USB port
USBHost myusb;
USBHub hub1(myusb);
MIDIDevice pushMidi(myusb);

int8_t octaveOffset = 3;
const int numberOfScales = 26;
const uint8_t scales[][numberOfScales] = { //define scales on the form 'semitones added to tonic'
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, // 0 'Chromatic'
  {0, 2, 4, 5, 7, 9, 11}, // 1 ‘Major’
  {0, 2, 3, 5, 7, 8, 10}, // 2 ‘Minor’
  {0, 2, 3, 5, 7, 9, 10}, // 3 ‘Dorian’
  {0, 2, 4, 5, 7, 9, 10}, // 4 ‘Mixolydian’
  {0, 2, 4, 6, 7, 9, 11}, // 5 ‘Lydian’
  {0, 1, 3, 5, 7, 8, 10}, // 6 ‘Phrygian’
  {0, 1, 3, 4, 7, 8, 10}, // 7 ‘Locrian’
  {0, 1, 3, 4, 6, 7, 9, 10}, // 8 ‘Diminished’
  {0, 2, 3, 5, 6, 8, 9, 11}, // 9 ‘Whole-half’
  {0, 2, 4, 6, 8, 10}, // 10 ‘Whole Tone’
  {0, 3, 5, 6, 7, 10}, // 11 ‘Minor Blues’
  {0, 3, 5, 7, 10}, // 12 ‘Minor Pentatonic’
  {0, 2, 4, 7, 9}, // 13 ‘Major Pentatonic’
  {0, 2, 3, 5, 7, 8, 11}, // 14 ‘Harmonic Minor’
  {0, 2, 3, 5, 7, 9, 11}, // 15 ‘Melodic Minor’
  {0, 1, 3, 4, 6, 8, 10}, // 16 ‘Super Locrian’
  {0, 1, 4, 5, 7, 8, 11}, // 17 ‘Bhairav’
  {0, 2, 3, 6, 7, 8, 11}, // 18 ‘Hungarian Minor’
  {0, 1, 4, 5, 7, 8, 10}, // 19 ‘Minor Gypsy’
  {0, 2, 3, 7, 8}, // 20 ‘Hirojoshi’
  {0, 1, 5, 7, 10}, // 21 ‘In-Sen’
  {0, 1, 5, 6, 10}, // 22 ‘Iwato’
  {0, 2, 3, 7, 9}, // 23 ‘Kumoi’
  {0, 1, 3, 4, 7, 8}, // 24 ‘Pelog’
  {0, 1, 3, 4, 5, 6, 8, 10} // 25 ‘Spanish’
};

const byte scaleSizes[numberOfScales] =
{ 12, 7, 7, 7, 7, 7, 7, 7, 8, 8, 6, 6, 5, 5,
  7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 6, 8
}; //havn't found a way to do this prettier yet

const byte numberOfLayouts = 6;
byte layouts[numberOfLayouts][2] = {
  {1, 3}, // forths up
  {3, 1}, // forths across
  {1, 2}, // thirds up
  {2, 1}, // thirds across
  {1, 8}, // seq up
  {8, 1}, // seq across
};

const int rootColor = 90;
const int regColor = 60;
const int inScaleColor = 78;

uint8_t currentScaleIndex = 1;

void setup()
{
	Serial.begin(115200);

	// turn led on
	pinMode(PIN_LED, OUTPUT); // LED pin
	digitalWrite(PIN_LED, HIGH);

	dinMidi.begin(MIDI_CHANNEL_OMNI);
	// Wait 1 second before turning on USB Host.  If connected USB devices
	// use too much power, Teensy at least completes USB enumeration, which
	// makes isolating the power issue easier.
	delay(1000);
	myusb.begin();
	// Wait 1 second so usb finishes initialization
	delay(1000);

	// put push into user mode
	uint8_t userModeMessage[9] = { 240, 71, 127, 21, 98, 0, 1, 0, 247 };
	pushMidi.sendSysEx(9, userModeMessage, true);

	// light up buttons under screen
	for (int i = 0; i < 120; i++)
	{
		pushMidi.sendControlChange(i, 127, 1);
	}

	// initialize pad leds
	/*
	  for (int i = 0; i < 64; i++)
	  {
	  uint8_t scaleOffset = ( (uint8_t)(i / 8) ) * (scaleSizes[currentScaleIndex] - 9);
	  uint8_t scaleNoteIndex = (i - scaleOffset) % scaleSizes[currentScaleIndex];
	  bool isNoteInScale = false;
	  for (int j = 0; j < scaleSizes[currentScaleIndex]; j++)
		pushMidi.sendNoteOn(i + 36, scales[currentScaleIndex][scaleNoteIndex] == 0 ? 67 : 88, 1); // channel 1 does solid color
	  }
	*/

	for (int i = 0; i < 64; i++)
	{
		int rowOffset = ((int)(i / 8)) * 3;
		int thePitch = i - rowOffset;

		bool isRootNote = (thePitch % 12) == 0;
		if (thePitch % 12 == 0) // if its the root note
		{
			pushMidi.sendNoteOn(i + 36, rootColor, 1);
		}
		else
		{
			bool isInCurrentScale = false;
			for (int scaleIndex = 0; scaleIndex < scaleSizes[currentScaleIndex]; scaleIndex++)
			{
				if (scales[currentScaleIndex][scaleIndex] == thePitch % 12)
				{
					isInCurrentScale = true;
					break;
				}
			}

			pushMidi.sendNoteOn(i + 36, isInCurrentScale ? inScaleColor : regColor, 1); // channel 1 does solid color

			//sendMidi(i + 36, isInCurrentScale ? inScaleColor : regColor, channel);
		}

	}
}

uint8_t getNoteFromScale(uint8_t data1)
{
	uint8_t i = data1 - 36;
	uint8_t scaleOffset = ((uint8_t)(i / 8)) * (scaleSizes[currentScaleIndex] - 9);
	uint8_t scaleIndex = (i - scaleOffset) % scaleSizes[currentScaleIndex];
	uint8_t octave = (i - scaleOffset) / scaleSizes[currentScaleIndex];
	uint8_t noteValue = (octave * 12) + scales[currentScaleIndex][scaleIndex] + (octaveOffset * 12);

	/*
	  Serial.print("i: ");
	  Serial.println(i);

	  Serial.print("scaleOffset: ");
	  Serial.println(scaleOffset);

	  Serial.print("scaleIndex: ");
	  Serial.println(scaleIndex);

	  Serial.print("octave: ");
	  Serial.println(octave);

	  Serial.print("note value: ");
	  Serial.println(noteValue);
	  Serial.println();
	*/

	return noteValue;
}

void loop()
{
	if (pushMidi.read())
	{
		uint8_t type = pushMidi.getType();   // status
		uint8_t data1 = pushMidi.getData1(); // note number
		uint8_t data2 = pushMidi.getData2(); // velocity
		uint8_t channel = pushMidi.getChannel();
		//const uint8_t *sys = pushMidi.getSysExArray();

		//Serial.println(data1);

		switch (type)
		{
		case midi::NoteOn:
			if (data1 > 35 && data1 < 100)
			{
				pushMidi.sendNoteOn(data1, data2, channel);

				uint8_t noteValue = getNoteFromScale(data1);
				dinMidi.sendNoteOn(noteValue, data2, channel);
			}

			break;

		case midi::NoteOff:
			if (data1 > 35 && data1 < 100)
			{
				pushMidi.sendNoteOff(data1, data2, channel);

				uint8_t noteValue = getNoteFromScale(data1);
				dinMidi.sendNoteOff(noteValue, data2, channel);
			}
			break;
		}
	}
}
