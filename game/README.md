# 🦆 STM32 Duck Hunt

![screenshot0.png](./Docs/screenshot0.png)

![screenshot1.png](./Docs/screenshot1.png)

![screenshot2.png](./Docs/screenshot2.png)

![gameplay.gif](./Docs/gameplay.gif)

- Gameplay Video: [gameplay_vid.mp4](./Docs/gameplay_vid.mp4)

Unity로 구현한 게임 Duck Hunt입니다. STM32 시리얼 통신을 통한 센서 입력 기반 슈팅 메커니즘을 포함합니다.

Duck Hunt implemented in Unity. It includes a shooting mechanism based on sensor input via STM32 serial communication.


## 🛠️ Tech Stack

- Engine: Unity 6.3 LTS

- Development Platform: Windows 11(AMD64)

- Target Platform

    - Windows(AMD64)
    
    - macOS(AMD64 / ARM64)

- **✨ Key Feature**

    - **STM32 시리얼 통신을 통한 센서 입력 기반 슈팅 메커니즘**

      **Shooting mechanism based on sensor input via STM32 serial communication**


## ❓ How to Run

1. [Releases](https://github.com/pnu-2025-esdl-wed-1/stm32-duck-hunt-game/releases) 페이지에서 빌드 파일 다운로드

   Download build files from the [Releases](https://github.com/pnu-2025-esdl-wed-1/stm32-duck-hunt-game/releases) page

2. 압축 해제

   Unzip the file

3. 실행 파일 실행

   Run the executable file

    - Windows(AMD64): `stm32-duck-hunt.exe`

    - macOS(AMD64 / ARM64): `stm32-duck-hunt.app`


## 🕹️ How to Play

0. SERIAL SETTINGS: 시리얼 통신 관련 설정. macOS 환경이나 마우스로 플레이 시 설정 불필요!

   Serial communication settings. Not required when playing on macOS or using a mouse!


    - COM 포트

      COM Port

    - 보드 레이트: 기본값 115200

      Baud rate: Default 115200

    - 임계값: HIT 판정을 위한 밝기 차이 경계값. 기본값 -20

      Threshold: Brightness difference boundary value for HIT detection. Default -20

1. START

    - 마우스 왼쪽 클릭: 사격

      Mouse left click: Shooting

    - 시리얼 통신을 통한 센서 입력: 사격

      Sensor input via serial communication: Shooting

    - ESC: 게임 종료

      Game exit

    - 규칙

      Rules
    
        - 잡을수록 속도가 빨라지는 오리를 맞추어 높은 점수를 기록하는 것을 목표로 합니다

          Aim to score high points by hitting ducks that accelerate the faster you shoot them

        - 3번의 사격 기회가 주어지며, 남은 사격 기회를 모두 소진할 경우 게임 오버가 됩니다

          You are given 3 shots. If you exhaust all remaining shots, it's game over

        - 연속으로 원샷킬을 달성하면 추가적인 점수 보정을 받습니다

          Achieving consecutive one-shot kills grants additional score bonuses


## 📄 Disclaimer

본 프로젝트는 Daniel The Fox님의 [Duck Hunt Remastered](https://danielthefox.itch.io/duck-hunt-remastered)에 영감을 받아 제작되었습니다.

This project was inspired by Daniel The Fox's [Duck Hunt Remastered](https://danielthefox.itch.io/duck-hunt-remastered).

- 사용된 리소스 목록

  Resources used

    - 오리, 개, 환경에 대한 스프라이트: [Pik on spriters-resource.com](https://www.spriters-resource.com/custom_edited/duckhuntcustoms/asset/63915/)

      The sprites for animated ducks, animated dog, and the environment: [Pik on spriters-resource.com](https://www.spriters-resource.com/custom_edited/duckhuntcustoms/asset/63915/)

    - 픽셀 스타일 폰트: [Big Blue Terminal Nerd Font Mono](./Assets/Fonts/BigBlueTerminal/LICENSE.TXT)

      Pixel-style font: [Big Blue Terminal Nerd Font Mono](./Assets/Fonts/BigBlueTerminal/LICENSE.TXT)

    - 배경 음악 및 효과음: [freesound.org](https://freesound.org/)

      Background music and sound effects: [freesound.org](https://freesound.org/)

---

본 게임은 야생 오리 사냥에 대한 묘사가 포함되어 있습니다. 동물 보호 단체와 조금이라도 관련 있는 모든 사용자들은 플레이를 삼가하시기를 정중히 권고합니다.

This game contains depictions of wild duck hunting. We respectfully advise all users with any connection to animal protection organizations to refrain from playing.
