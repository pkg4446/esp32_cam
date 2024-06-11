const express = require('express');
const bodyParser  = require('body-parser');
const WebSocket   = require('ws');
const app   = express();
const http  = require('http').Server(app);
const fs    = require('fs');

// WebSocket 서버 생성
const socket  = new WebSocket.Server({ port: 3000 });
const clients = new Map();
const cameras = new Map();
const users   = new Map();
// 클라이언트 연결 시 처리
socket.on('connection', function connection(ws) {
  // 클라이언트별 ID 생성 (예: 클라이언트 IP 주소)
  const client_id = ws._socket.remoteAddress+":"+Math.floor(Math.random() * 10);
  let   MAC_addr  = "";
  clients.set(client_id, ws); 
  console.log("ID:"+client_id);
  // Arduino로부터 전송된 바이너리 데이터를 수신하고 처리
  ws.on('message', function incoming(data) {
    if(data.length<999){
      console.log("Incoming message: %s", data);
      if(data.slice(0,4)=="MAC:" || data.slice(0,4)=="CAM:"){
        MAC_addr = data.slice(4).toString();
        if(data.slice(0,4)=="MAC:"){cameras.set(MAC_addr,client_id);}
        else{
          if(users.has(MAC_addr)){
            const user_id = users.get(MAC_addr);
            if(clients.has(user_id)){
              const targetClient = clients.get(user_id);
              targetClient.send("interrupted");
            }
          }
          users.set(MAC_addr,client_id);
          if (cameras.has(MAC_addr)){
            const camera_id = cameras.get(MAC_addr);
            if(clients.has(camera_id)){
              const targetClient = clients.get(camera_id);
              targetClient.send("cam on\n");
            }
          }
        }
      }
    }else{
      // 특정 클라이언트에게 바이너리 데이터 전송
      if (users.has(MAC_addr)){
        const user_id = users.get(MAC_addr);
        if(clients.has(user_id)){
          const targetClient = clients.get(user_id);
          targetClient.send(data);
        }
      }
    }
  });

  ws.on('close', function() {
    console.log('Client disconnected');
    // 클라이언트가 연결을 끊었을 때 메시지 보내기
    if (users.has(MAC_addr) && users.get(MAC_addr) == client_id){
      if (cameras.has(MAC_addr)){
        const camera_id = cameras.get(MAC_addr);
        if(clients.has(camera_id)){
          const targetClient = clients.get(camera_id);
          targetClient.send("cam off\n");
        }
      }
    }
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