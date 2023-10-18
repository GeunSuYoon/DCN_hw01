# University_homework
chmod +x ./http_server* 명령어를 사용해 http_server로 시작하는 이름의 파일들에 실행 권한 부여
./http_server_<YOUR_SYSTEM> 62123 명령어를 이용해 서버를 켠다 (YOUR_SYSTEM : apple_silicom, intel_mac, linux, WSL)
http://127.0.0.1:62123 으로 웹페이지 접근!

## B. Behavior
   1. View Album! 버튼을 클릭하면 web album에 12개의 이미지가 나온다. (초기 이미지)
   2. POST Image에 Browse버튼을 누르면 1MB 이하의 이름이 영어, 숫자로 된 .jpg 이미지를 업로드 할 수 있다.
   3. Go nowhere! 버튼을 누르면 404 Not Found 에러가 출력된다.
   4. Autheticate! 버튼을 누르고 username에 DCN, password에 FALL2023을 입력하면 secret image(초기값 : 교수님 사진)가 출력된다.

## C. Implementation Objectives
   서버는 아래 일들을 수행한다.
   1. Listening soket을 생성해 웹 브라우저의 연결을 받아들임
   2. HTTP 요청을 수신하고 구문을 분석함
   3. 요청에 따른 적절한 동작 수행
   4. 수행한 작업을 기반으로 HTTP 응답을 생성
   5. HTTP 응답을 웹 브라우저로 return
   2 ~ 5가 반복되며, http_engine.c 파일만 수정하면 된다.
   Details
   1. Listening soket 생성과 연결 수락
      - 서버가 시작될 때 main function이 http_engine.c 파일 내부의 server_engine() 함수를 부른다.
        TCP 소켓을 초기화하고 server_port argument에 묶는다.
      무한루프 내에서 보내지는 연결을 받아들이며 server_routine() 함수를 이용해 서버와 연결을 처리한다.
   2. HTTP 요청 받고 분석
      - HTTP 요청은 아래 사진을 따른다.
![University_homework/assets/146644182/48711af8-f30c-43d7-84bd-11ca1e97bce0](https://github.com/GeunSuYoon/University_homework/assets/146644182/48711af8-f30c-43d7-84bd-11ca1e97bce0)
      - sp는 ' ', cr는 '\r', 그리고 if는 '\n'
      - server_routine 함수 내에 header_buffer 문자 배열과 무한 루프가 있다.
        HTTP header를 1. 헤더 끝에 구분기호(i.e., \r\n\r\n)가 수신될 때, 2. 에러가 발생하거나 클라이언트가 연결을 해재할 때, 3. 클라이언트로 부터 너무 긴 헤더 메세지가 올 때 까지 루프 내에서 수신한다.
      - 헤더를 성공적으로 수신한 뒤, 클라이언트의 요청이 무엇인지 알기 위해 메세지를 분석한다. 받은 헤더 문자열로 부터 method, URL, 그리고 header field name & header field value tuple을 추출해야 한다.
      - 헤더 문자열을 http_t struct로 변환하기 위해 parse_http_header() 함수 구현을 추천한다. http_t struct는 http_util.c에서 제공하는 다양한 HTTP 요소를 관리하고 다루는 것을 도와주는 C 구조체다.
     다만, 필수적으로 해야하는 것은 아니다.
   3. 적절한 행동 취하기
      - 웹 브라우저에서 온 요청은 GET이나 POST method일 것이다.
        GET method는 HTML이나 CSS 파일과 같은 웹 페이지 요소를 웹 서버로부터 검색하기 위해 사용된다.
        POST method는 유저 이미지와 같은 클라이언트로부터 온 data를 HTTP 서버로 업로드하기 위해 사용된다.
      - GET method에서 HTTP 요청과 같은 URL로부터 요청된 파일을 return 해야한다.
      - POST method에서 HTTP 요청의 body에서 파일을 검색하고, 서버 내부에 파일로 저장하고, web album에 새롭게 올라온 파일을 보여줄 수 있도록 HTML 파일로 업로드해야한다.
      - 서버 기능을 지원할 것으로 예상되는 경우는 http_engine.c에서 주석화 되어 있으니 참고하자!
      - GET을 이용한 업로드는 이번 프로젝트에서 다루지 않는다.
   4. HTTP response 생성
      - 클라이언트 요청에 따라 해야할 행동을 받아 그에 따른 HTTP response를 웹 브라우저에 다시 보내야 할 것이다. http_t struct와 도움 함수들을 사용하자.
        response는 init_http_with_arg() 함수와 적절한 response 코드, response에 옳바른 header field 와 body를 붙여 초기화 해주어야 한다.
      - http_t struct와 HTTP response를 생성하는 함수의 예시는 "431 Header too large" 응답을 보낼 때 http_engine.c파일에 포함되어 있다.
   5. Returnning HTTP response
      - HTTP 응답이 준비되었을 때, 클라이언트에 response를 다시 보내야한다.
        The response massage는 비슷한 HTTP 요청 massage와 비슷한 구조지만, 요청 줄 대신 상태 줄을 사용한 HTTP protocol 표준 형태를 따라야 한다.
![University_homework/assets/146644182/1ded4fa8-cfa8-43e9-bd41-4a85ade01de5](https://github.com/GeunSuYoon/University_homework/assets/146644182/1ded4fa8-cfa8-43e9-bd41-4a85ade01de5)
      - 다행히 해당 부분은 이미 구현되어있다. http_t struct를 옳바른 HTTP response massage로 바꾸고 웹 브라우저로 전송한다. write_http_to_buffer() 함수를 읽어보면 동작원리가 이해가 갈 것이며, parse_http_header() 함수 구현에 도움이 될 것이다!

## D. 중요한 부분
   - 서버를 빌드하기 위해, make를 타이핑 해라. 코드를 바꾸고 난 뒤에는 다시 make 해줘야 한다. ctrl + c로 서버 프로그램을 닫자.
   - 함수의 return value, name 그리고 정의된 함수의 arguments는 바꾸면 안 된다!
   - 테스트 목적으로 다른 파일이나 함수를 정의하거나 만들 수 있지만, "http_engine.c" 파일만 제출해야 한다!
   - 필요하다면 http_engine.c 내부에 새로운 함수, 전역 변수, enum, struct등을 정의할 수 있다. 또한, C/POSIX 표준으로 정의된 다른 라이브러리를 사용할 수 있다. 설치가 필요한 외부 라이브러리 사용은 추천하지 않는다!
   - "http_engine.c" 내부 comments를 주의깊게 읽어라. http_t struct와 helper function을 잘 이용하면 HTTP 요소들의 관리와 조작이 쉬워질 것이다.
   - 또한, 웹 브라우저의 개발자 도구(Web Developer Tools) 또한 서버 디버깅에 도움을 줄 것이다. 네트워크 탭은 너의 서버와 웹 브라우저 사이 HTTP request 메시지와 HTTP response 메시지 교환을 보여줄 것이다!
   - 메모리 관리는 성적 반영 X. 하지만 프로젝트가 많은 양의 동적할당과 소켓과 파일 사이의 읽고 쓰는 것을 요구하므로 C 포인터와 메모리 구조를 이해하는 것은 좋다!
   - HTML, CSS 또는 JS와 같은 프론트 엔드 웹 페이지 요소들은 웹에 중요한 부분이다. 하지만, 이번 과제에서 요구하진 않는다!

## Aplication 2: BitTorrent-like P2P file sharing

## Introduction to Torrent Application
![University_homework/assets/146644182/28973d8a-0a3b-4bcc-8344-521d7270985f](https://github.com/GeunSuYoon/University_homework/assets/146644182/28973d8a-0a3b-4bcc-8344-521d7270985f)
   - 상기 사진에 있는 붉은색 요소들을 사용할 것이다!
   - 각 토렌트 어플들은 토렌트(토렌트 데이터베이스)의 목록, 클라이언트 함수, 서버 함수를 가지고 있다.
   - 각 토렌트는 파일을 32 킬로바이트의 블록으로 나눌 것이다. 클라리언트 함수는 토렌트 내부의 데이터베이스를 되풀이하고, 토렌트 블록을 다운하기 위해 요구되는 행동(해당 블록에 있는 것으로 알려져있는 peer에 블록 데이터를 요구)을 수행할 것이다.
   - 서버 함수는 원격 peer의 메세지를 받고 연결의 명령에 따라 적절한 조치를 취할 것이다(HTTP 서버가 하는 것 처럼). A request message, a push massge 이라는 두 다른 타입의 메세지를 서버가 받을 것이다.
   - A request massage는 원격 peer의 클라이언트 함수로부터 보내지고 특정 토렌트 요소를 요구할 것이다. 원격 peer가 요구하는 토렌트 요소는 네 가지가 있다. A request massage를 받은 후, 서버 함수는 적절한 handler를 불러 일치하는 push massage가 요구하는 요소를 반환할 것이다.
      1. 토렌트 정보(파일 이름, 사이즈 등)
      2. 해당 토렌트에 알려진 peer 목록
      3. Peer의 최근 다운로드 상태
      4. 블록 데이터
   - A push massage는 원격 peer의 서버 함수로부터 보내지고 클라이언트 함수가 요구하는 토렌트 요소르 가지고 있다. 원격 peer가 push할 수 있는 토렌트 요소는 네 가지가 있다. A push massage가 도착했을 때, 서버 함수는 해당 massage에 적절한 handler를 부르고 pushed element를 저장하기 위해 토렌트 데이터베이스에 업데이트 할 것이다.
   - 
   
