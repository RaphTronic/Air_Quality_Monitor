# DIY Air Quality Monitor & Datalogger: PM1-10, VOC, NOx, CO2

This repository contains the **Arduino code** and the **optional CAD design files** to build the V1 version of the DIY Air Quality Monitor & Datalogger described in this series of posts:
* Part 1 - **Introduction**: https://raphtronic.blogspot.com/2024/08/diy-pm-voc-air-quality-monitor-with.html
* Part 2 - **Implementation**:      _in the works (describes the files uploaded here)_
* Part 3 - **Results**:  _a month away_

**Basic Arduino sketches** to test and get started with each sensor / interface (PM, CO2, RTC, uSD card, battery...) will also be uploaded. Along with **basic applications**, like saving a color-accurate screenshot to the uSD card, that I could not find anywhere else. The goal here is to help people learn these capabilities, explore or debug their setup for this project, leverage simple code for their own projects.

Expectations for code updates: V1 is 95% core-features complete at this time (Aug'24). The remaining TODOs are listed at the beginning of the code. Almost weekly updates will be committed over the next 3 to 6 months (mostly code cleanup & non-essential features). Focus will then shift to V2 (wireless sensors).

For more details see the posts listed above.

Happy coding & clean air to all !
** **
Warning: _I am by no means a pro SW developer. Which explains the bloated, unstructured, confusing, giant single-file piece of spaghetti code that runs this monitor. It is also written in an unsavory mix of Arduino / C / C++. Good luck to anyone trying to mod anything... Submit a github issue if you have questions or feature requests._
** **

![20240812_121825 Air Quality Index](https://github.com/user-attachments/assets/c70e8b9c-75d2-4e7d-b0cb-0024cb651dc5)

![Assembled CA](https://github.com/user-attachments/assets/638d40f3-8258-47c4-82fe-a588055b3c21)

![Architecture diagram](https://github.com/user-attachments/assets/49711054-e817-4ed8-9b47-8c0bdb0f40cf)

![screenshot3 chart sen55](https://github.com/user-attachments/assets/4786bc32-a46e-4b42-89d1-75a3057155c2)
