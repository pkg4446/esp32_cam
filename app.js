const express = require('express');
const bodyParser  = require('body-parser');
const WebSocket   = require('ws');
const app   = express();
const http  = require('http').Server(app);
const fs    = require('fs');

// WebSocket 서버 생성
const wss = new WebSocket.Server({ port: 3000 });
// 클라이언트 연결 시 처리
wss.on('connection', function connection(ws) {
  console.log('Client connected');

  // 클라이언트별 ID 생성 (예: 클라이언트 IP 주소)
  const clientId = "test";
  //const clientId = ws._socket.remoteAddress;
  // Arduino로부터 전송된 바이너리 데이터를 수신하고 처리
  ws.on('message', function incoming(data) {
    // 특정 클라이언트에게만 데이터 전송
    wss.clients.forEach(function each(client) {
      if (client.readyState === WebSocket.OPEN && "test" === clientId) {
        client.send(data);
      }
    });
  });

  ws.on('close', function() {
    console.log('Client disconnected');
    // 클라이언트가 연결을 끊었을 때 메시지 보내기
    wss.clients.forEach(function each(client) {
      if (client.readyState === WebSocket.OPEN && clientId === "test") {
        client.send("Goodbye");
      }
    });
  });

});

const imageDir = './images';
// 디렉토리가 없으면 생성
if (!fs.existsSync(imageDir)){
    fs.mkdirSync(imageDir);
}
// 클라이언트에게 정적 파일 제공 (영상을 표시하기 위해)
app.use(express.static('public'));
// 클라이언트가 웹 페이지에 접속했을 때
app.get('/', (req, res) => {
  res.sendFile(__dirname + '/index.html');
});
app.use(bodyParser.raw({ type: 'image/jpeg', limit: '10mb' }));
// POST 요청 처리
app.post('/upload', (req, res) => {
  // 이미지 데이터를 파일로 저장
  const fileName = `${imageDir}/image_${Date.now()}.jpg`;
  fs.writeFile(fileName, req.body, 'binary', (err) => {
      if (err) {
          console.error('Error saving image:', err);
          res.status(500).send('Error saving image');
      } else {
          console.log('Image saved successfully:', fileName);
          res.status(200).send('Image saved successfully');
      }
  });
});

// HTTP 서버 생성 및 실행
const PORT = 3004;
http.listen(PORT, () => {
  console.log(`Server is running on http://localhost:${PORT}`);
});