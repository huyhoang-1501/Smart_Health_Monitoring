// ================== LOAD AI MODEL ==================
let aiModel = null;
let aiScaler = null;

const aiLabels = [
  'Bình thường',
  'Hoạt động nhẹ',
  'Hoạt động trung bình',
  'Hoạt động mạnh',
  'Cảnh báo sức khỏe không ổn định',
  'Nhịp tim cao bất thường',
  'Sức khỏe siêu tốt',
  'Cảnh báo nguy hiểm'
];

async function loadAIModel() {
  const status = document.getElementById('ai-status');
  if (!status) return;

  status.textContent = "Đang tải AI model...";
  status.style.color = "orange";

  try {
    console.log("Bắt đầu load model...");
   aiModel = await tf.loadLayersModel('tfjs_model/model.json');
    console.log("Model AI loaded thành công!");

    console.log("Bắt đầu load scaler...");
    const scalerResp = await fetch('scaler.json');
    if (!scalerResp.ok) throw new Error(`Không tìm thấy scaler.json (HTTP ${scalerResp.status})`);
    const scalerData = await scalerResp.json();
    aiScaler = scalerData;
    console.log("Scaler loaded:", aiScaler);

    status.textContent = "AI sẵn sàng! Dự đoán ngay";
    status.style.color = "green";
  } catch (err) {
    console.error("Lỗi load AI:", err);
    status.textContent = "Lỗi: " + err.message;
    status.style.color = "red";
    status.title = err.message;
  }
}

loadAIModel();

// ================== TOGGLE MENU ==================
document.getElementById("logo").addEventListener("click", () => {
  document.getElementById("logo-menu").classList.toggle("hidden");
});

// ================== CẬP NHẬT THỜI GIAN ==================
function updateTime() {
  const now = new Date();
  const dateStr = now.toLocaleDateString('vi-VN', {
    day: '2-digit',
    month: '2-digit',
    year: 'numeric'
  });
  const timeStr = now.toLocaleTimeString('vi-VN', {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    hour12: false
  });

  document.getElementById('date').textContent = dateStr;
  document.getElementById('time').textContent = timeStr;
}

updateTime();
setInterval(updateTime, 1000);

// ================== HIỆN SECTION ==================
function showSection(section) {
  document.getElementById("logo-menu").classList.add("hidden");
  document.querySelectorAll(".section").forEach(sec => sec.classList.add("hidden"));

  const target = document.getElementById(section + "-section");
  if (target) target.classList.remove("hidden");

  // === NHẬP SỐ ĐIỆN THOẠI ===
  if (section === "input") {
    phoneInput.value = "";
    statusText.textContent = "";
  }

  // === HIỂN THỊ DỮ LIỆU ===
  if (section === "display") {
    firebase.database().ref("user/phone/sdt").once("value")
      .then(snapshot => {
        const data = snapshot.val();
        const output = document.getElementById("data-output");
        output.textContent = data
          ? `Số điện thoại đã lưu: ${data}`
          : "Chưa có số điện thoại nào được lưu.";
      })
      .catch(error => {
        document.getElementById("data-output").textContent = "Lỗi tải dữ liệu: " + error.message;
      });
  }

  // === AI: TỰ ĐỘNG ĐIỀN DỮ LIỆU TỪ CẢM BIẾN ===
  if (section === "ai") {
    firebase.database().ref("parameter").once("value").then(snap => {
      const data = snap.val();
      if (data) {
        const bpmInput = document.getElementById('ai-bpm');
        const spo2Input = document.getElementById('ai-spo2');
        if (bpmInput) bpmInput.value = data.heartbeat ?? '';
        if (spo2Input) spo2Input.value = data.spo2 ?? '';
      }
    }).catch(err => {
      console.error("Lỗi lấy dữ liệu cho AI:", err);
    });
  }
}

// ================== FIREBASE LƯU SỐ ĐIỆN THOẠI ==================
const phoneForm = document.getElementById("phone-form");
const phoneInput = document.getElementById("phone-input");
const statusText = document.getElementById("status");
const deleteBtn = document.getElementById("delete-btn");

phoneForm.addEventListener("submit", function(e) {
  e.preventDefault();
  const phone = phoneInput.value.trim();
  if (phone === "") {
    statusText.textContent = "Please enter the number phone!";
    statusText.style.color = "yellow";
    return;
  }

  firebase.database().ref("user/phone/sdt").set(phone)
    .then(() => {
      statusText.textContent = "Save successfully";
      statusText.style.color = "green";
      phoneInput.value = "";
    })
    .catch((error) => {
      statusText.textContent = "Error: " + error.message;
      statusText.style.color = "red";
    });
});

deleteBtn.addEventListener("click", function() {
  firebase.database().ref("user/phone/sdt").remove()
    .then(() => {
      statusText.textContent = "The phone number has been deleted!";
      statusText.style.color = "orange";
      phoneInput.value = "";
    })
    .catch((error) => {
      statusText.textContent = "Error: " + error.message;
      statusText.style.color = "red";
    });
});

// ================== CẤU HÌNH FIREBASE ==================
const firebaseConfig = {
  apiKey: "AIzaSyD3_MWJ-A5wkar9UdDEjo0EuTTmmjxs-vo",
  authDomain: "project-2-health.firebaseapp.com",
  databaseURL: "https://project-2-health-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "project-2-health",
  storageBucket: "project-2-health.appspot.com",
  messagingSenderId: "804061608537",
  appId: "1:804061608537:web:9c3f975c4d761f48b64f48",
  measurementId: "G-H421Q9D8ZC"
};
firebase.initializeApp(firebaseConfig);
const db = firebase.database();

// ================== HIỂN THỊ DỮ LIỆU CẢM BIẾN + BIỂU ĐỒ ==================
const el = {
  heartbeat: document.getElementById('heartbeat'),
  steps: document.getElementById('steps'),
  spo2: document.getElementById('spo2')
};

let sensorData = { heartbeat: [], steps: [], spo2: [], labels: [] };

// Chart Nhịp tim
const heartChart = new Chart(document.getElementById('heartChart').getContext('2d'), {
  type: 'line',
  data: {
    labels: sensorData.labels,
    datasets: [{
      label: 'BPM',
      data: sensorData.heartbeat,
      borderColor: '#ff6b6b',
      backgroundColor: 'rgba(255,107,107,0.2)',
      fill: true,
      tension: 0.3
    }]
  },
  options: { responsive: true, scales: { x: { ticks: { display: false } } } }
});

// Chart Bước chân
const stepChart = new Chart(document.getElementById('stepChart').getContext('2d'), {
  type: 'line',
  data: {
    labels: sensorData.labels,
    datasets: [{
      label: 'Steps',
      data: sensorData.steps,
      borderColor: '#4dabf7',
      backgroundColor: 'rgba(77,171,247,0.2)',
      fill: true,
      tension: 0.3
    }]
  },
  options: { responsive: true, scales: { x: { ticks: { display: false } } } }
});

// Chart SpO₂
const spo2Chart = new Chart(document.getElementById('spo2Chart').getContext('2d'), {
  type: 'line',
  data: {
    labels: sensorData.labels,
    datasets: [{
      label: 'SpO₂ (%)',
      data: sensorData.spo2,
      borderColor: '#51cf66',
      backgroundColor: 'rgba(81,207,102,0.2)',
      fill: true,
      tension: 0.3
    }]
  },
  options: { responsive: true, scales: { x: { ticks: { display: false } } } }
});

// Lắng nghe dữ liệu từ Firebase
db.ref('parameter').on('value', snap => {
  const data = snap.val() || { heartbeat: 0, steps: 0, spo2: 0 };

  el.heartbeat.textContent = `${data.heartbeat} bpm`;
  el.steps.textContent = data.steps;
  el.spo2.textContent = `${data.spo2} %`;

  sensorData.heartbeat.push(data.heartbeat);
  sensorData.steps.push(data.steps);
  sensorData.spo2.push(data.spo2);
  sensorData.labels.push("");

  if (sensorData.heartbeat.length > 10) {
    sensorData.heartbeat.shift();
    sensorData.steps.shift();
    sensorData.spo2.shift();
    sensorData.labels.shift();
  }

  heartChart.data.labels = sensorData.labels;
  heartChart.data.datasets[0].data = sensorData.heartbeat;
  heartChart.update();

  stepChart.data.labels = sensorData.labels;
  stepChart.data.datasets[0].data = sensorData.steps;
  stepChart.update();

  spo2Chart.data.labels = sensorData.labels;
  spo2Chart.data.datasets[0].data = sensorData.spo2;
  spo2Chart.update();
});

// ================== AI DỰ ĐOÁN ==================
document.getElementById('ai-predict-btn')?.addEventListener('click', async () => {
  const resultEl = document.getElementById('ai-result-full');
  
  if (!aiModel || !aiScaler) {
    resultEl.textContent = "Waiting for AI loading.....";
    resultEl.style.color = "red";
    return;
  }

  const bpm = parseFloat(document.getElementById('ai-bpm').value) || 0;
  const spo2 = parseFloat(document.getElementById('ai-spo2').value) || 0;

  if (bpm < 30 || bpm > 200 || spo2 < 70 || spo2 > 100) {
    resultEl.textContent = "Invalid data !";
    resultEl.style.color = "red";
    return;
  }

  const x1 = (bpm - aiScaler.mean[0]) / aiScaler.scale[0];
  const x2 = (spo2 - aiScaler.mean[1]) / aiScaler.scale[1];

  const pred = aiModel.predict(tf.tensor2d([[x1, x2]])).argMax(-1).dataSync()[0];
  const label = aiLabels[pred];

  resultEl.textContent = label;
  const colors = {
    'Bình thường': '#27ae60',
    'Hoạt động nhẹ': '#f39c12',
    'Hoạt động trung bình': '#e67e22',
    'Hoạt động mạnh': '#d35400',
    'Cảnh báo sức khỏe không ổn định': '#e74c3c',
    'Nhịp tim cao bất thường': '#c0392b',
    'Sức khỏe siêu tốt': '#2ecc71',
    'Cảnh báo nguy hiểm': '#8e44ad'
  };
  const color = colors[label] || '#2c3e50';
  resultEl.style.color = 'white';
  resultEl.style.background = color;
  resultEl.style.border = 'none';
  resultEl.style.boxShadow = `0 0 20px ${color}80`;
});