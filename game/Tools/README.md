# 시리얼 브리지

.NET 프레임워크의 `System.IO.Ports`를 사용할 수 없는 macOS를 지원하기 위하여 추가한 시리얼 브리지입니다. `pyserial`을 통해 Unity 클라이언트와 시리얼 간 데이터 송수신을 브리징합니다.


## 실행 방법

스크립트 실행을 위해 Python과 필수 패키지가 설치되어 있어야 합니다. 그냥 `pip install -r requirements.txt`로 설치하셔도 되긴하는데, '내 Python 전역 환경을 더럽히기 싫어!'하시는 분들은 아래와 같이 가상 환경 생성해서 설치하세요!

```Shell
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

- 가상 환경 비활성화

  ```Shell
  deactivate
  ```
