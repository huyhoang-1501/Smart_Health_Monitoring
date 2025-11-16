// Toggle menu khi nhấn logo
document.getElementById("logo").addEventListener("click", () => {
  document.getElementById("logo-menu").classList.toggle("hidden");
});

// Cập nhật thời gian thực ở header
const timeElement = document.getElementById('header-title');
// CẬP NHẬT THỜI GIAN - CÓ : - KHOẢNG CÁCH BÌNH THƯỜNG
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

  // Gán trực tiếp
  document.getElementById('date').textContent = dateStr;
  document.getElementById('time').textContent = timeStr;
}

// Chạy ngay + mỗi giây
updateTime();
setInterval(updateTime, 1000);
// Hiện nội dung theo lựa chọn
function showSection(section) {
  // Ẩn menu
  document.getElementById("logo-menu").classList.add("hidden");

  // Ẩn hết section, bao gồm welcome
  document.querySelectorAll(".section").forEach(sec => sec.classList.add("hidden"));

  // Hiện section được chọn
  const target = document.getElementById(section + "-section");
  if (target) {
    target.classList.remove("hidden");
  }

  // Reset input khi mở input-section
  if (section === "input") {
    phoneInput.value = "";
    statusText.textContent = "";
  }

  // Nếu chọn display thì load dữ liệu
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
}

// ================== Firebase lưu số điện thoại ==================
const phoneForm = document.getElementById("phone-form");
const phoneInput = document.getElementById("phone-input");
const statusText = document.getElementById("status");
const deleteBtn = document.getElementById("delete-btn");

// Sự kiện lưu số điện thoại
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

// Sự kiện xoá số điện thoại
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

// Cấu hình Firebase
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

// HTML elements
const el = {
  heartbeat: document.getElementById('heartbeat'),
  steps: document.getElementById('steps'),
  spo2: document.getElementById('spo2')
};

// Dữ liệu
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

// Lấy dữ liệu từ Firebase
db.ref('parameter').on('value', snap => {
  const data = snap.val() || { heartbeat: 0, steps: 0, spo2: 0 };

  // Hiển thị giá trị
  el.heartbeat.textContent = `${data.heartbeat} bpm`;
  el.steps.textContent = data.steps;
  el.spo2.textContent = `${data.spo2} %`;

  // Thêm vào mảng
  sensorData.heartbeat.push(data.heartbeat);
  sensorData.steps.push(data.steps);
  sensorData.spo2.push(data.spo2);
  sensorData.labels.push("");

  if(sensorData.heartbeat.length > 10){
    sensorData.heartbeat.shift();
    sensorData.steps.shift();
    sensorData.spo2.shift();
    sensorData.labels.shift();
  }

  // Cập nhật chart
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