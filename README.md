# University_homework
chmod +x ./http_server* 명령어를 사용해 http_server로 시작하는 이름의 파일들에 실행 권한 부여
./http_server_<YOUR_SYSTEM> 62123 명령어를 이용해 서버를 켠다 (YOUR_SYSTEM : apple_silicom, intel_mac, linux, WSL)
http://127.0.0.1:62123 으로 웹페이지 접근!

B. Behavior
   1. View Album! 버튼을 클릭하면 web album에 12개의 이미지가 나온다. (초기 이미지)
   2. POST Image에 Browse버튼을 누르면 1MB 이하의 이름이 영어, 숫자로 된 .jpg 이미지를 업로드 할 수 있다.
   3. Go nowhere! 버튼을 누르면 404 Not Found 에러가 출력된다.
   4. Autheticate! 버튼을 누르고 username에 DCN, password에 FALL2023을 입력하면 secret image(초기값 : 교수님 사진)가 출력된다.

C. Implementation Objectives
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
<img width="454" alt="스크린샷 2023-10-15 오후 1 27 09" src="https://github.com/GeunSuYoon/University_homework/assets/146644182/9d93c8e6-51c3-4a5a-be75-34da8cc1d889">
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
<img width="642" alt="image" src="https://github.com/GeunSuYoon/University_homework/assets/146644182/1ded4fa8-cfa8-43e9-bd41-4a85ade01de5">
