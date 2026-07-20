# supra
supra synth

# NOTES 

note that the whole repo uses old naming conventions 

SQUIGL = STAMP 
SEQO = ABACUS
MORSD = CRYPTEX 
MELO = CHIME

Lots of stuff here is very messy and unoptimized 

## BUILDING

The firmware source and the library code it uses are included directly in this
directory. `u8g2/csrc`, `pico-littlefs/littlefs-lib`, and
`pico-littlefs/stdinit-lib` are vendored so this directory does not rely on
Git submodules.

Install the ARM embedded toolchain and Raspberry Pi Pico SDK, then point CMake
at the SDK. This source has been tested with Pico SDK 2.2.0.

```sh
cmake -S . -B build -DPICO_SDK_PATH=/path/to/pico-sdk
cmake --build build --parallel
```

The flashable firmware is written to `build/supra.uf2`.

AI DISCLOSURE 

This was a super ambitious project for me and parts of it required me to get a little beyond my abilities as a software engineer. I used AI for parts of the programming, I'm not sure how I feel about. I don't use AI to ideate or assist with design decisions, because I don't like doing that. I don't use it to whole cloth create something, because I don't like that. It's really nice to be able to offload tasks that I truly don't care about or have any artistic interest in how their done (Ie, 2 different ways to dim an LED, which I will explain later). Part of me wonders if it would have been better to be less ambitious and write more by hand, but that feels like its over emphasizing purity. Everything was at least prototyped by hand initially. most of the use of AI for programming was for stuff like the following situations: 

I had a lot of trouble using the DAC in supra with RP350 (my main chip) and after lots of troubleshooting I ended up using a proprietary driver that AI wrote to interface with the DAC because it worked where off the shelf libraries didn't (writing that myself would have been incredible hard, taken a long time, and not have been knowledge I would need in the future) 

I realized that because of a small quirk in the way I set up the hardware on the LEDs that one of them would not have dimming capabilities unless I redesigned and ordered a new batch, or if I implemented a software solution using PIO which is a more advanced feature of my main chip that I haven't gone into or learned about yet. So I had AI write it to save the time and materials of a new batch. 

I used AI to rewrite parts of the code that were innificient and to implement multithreading which I tried to do on my own but failed and was frustrated since It was a lot of work, and the only thing it would do is make things run smoother, so I wasn't doing anything artistic or interesting design wise. I decide dto allow myself to offload that task 

etc. etc.  

you will notice AI comments around the repo as I used AI to implement some features on many modes like self clocking etc. which would have been out of scope to add myself late in production 
