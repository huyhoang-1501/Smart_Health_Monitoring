// ================== LOAD AI MODEL – DÙNG ONNX (ỔN ĐỊNH NHẤT) ==================
let aiModel = null;       
let onnxSession = null;    
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

  status.textContent = "Đang tải AI (ONNX)...";
  status.style.color = "orange";

  try {
    onnxSession = await ort.InferenceSession.create('hr_spo2_model.onnx');
    console.log("Model ONNX đã tải thành công!");

    const scalerResp = await fetch('scaler.json');
    if (!scalerResp.ok) throw new Error("Không tìm thấy scaler.json");
    aiScaler = await scalerResp.json();
    console.log("Scaler loaded thành công!");

    status.textContent = "AI sẵn sàng! Dự đoán ngay";
    status.style.color = "green";

    setTimeout(() => {
      const predictBtn = document.getElementById('ai-predict-btn');
      if (predictBtn) predictBtn.click();
    }, 800);

  } catch (err) {
    console.error("Lỗi khi tải AI:", err);
    status.textContent = "Lỗi AI: " + err.message;
    status.style.color = "red";
    setTimeout(loadAIModel, 5000);
  }
}
loadAIModel();

// ================== TỰ ĐỘNG DỰ ĐOÁN KHI CÓ DỮ LIỆU TỪ FIREBASE ==================
function autoPredict() {
  const bpmInput = document.getElementById('ai-bpm');
  const spo2Input = document.getElementById('ai-spo2');
  const predictBtn = document.getElementById('ai-predict-btn');

  if (onnxSession && aiScaler && bpmInput?.value && spo2Input?.value && predictBtn) {
    predictBtn.click();
  } else {
    setTimeout(autoPredict, 500);
  }
}

// ================== TOGGLE MENU KHI NHẤN LOGO ==================
document.getElementById("logo").addEventListener("click", () => {
  document.getElementById("logo-menu").classList.toggle("hidden");
});

// ================== CẬP NHẬT NGÀY GIỜ ==================
function updateTime() {
  const now = new Date();
  const dateStr = now.toLocaleDateString('vi-VN', { day: '2-digit', month: '2-digit', year: 'numeric' });
  const timeStr = now.toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false });

  document.getElementById('date').textContent = dateStr;
  document.getElementById('time').textContent = timeStr;
}
updateTime();
setInterval(updateTime, 1000);

// ================== HIỂN THỊ SECTION ==================
function showSection(section) {
  document.getElementById("logo-menu").classList.add("hidden");
  document.querySelectorAll(".section").forEach(sec => sec.classList.add("hidden"));
  const target = document.getElementById(section + "-section");
  if (target) target.classList.remove("hidden");

  if (section === "input") {
    document.getElementById("phone-input").value = "";
    document.getElementById("status").textContent = "";
  }

  if (section === "ai") {
    // Lấy dữ liệu realtime từ project-2-health và tự động dự đoán
    db.ref("parameter").on("value", snap => {
      const data = snap.val();
      if (data && data.heartbeat !== undefined && data.spo2 !== undefined) {
        document.getElementById('ai-bpm').value = data.heartbeat;
        document.getElementById('ai-spo2').value = data.spo2;
        autoPredict();
      }
    }, err => console.error("Lỗi realtime AI:", err));
  }
}

// ================== PROJECT 1: AUTH (LOGIN GOOGLE + EMAIL) - project2-98ad4 ==================
const firebaseConfigAuth = {
  apiKey: "AIzaSyDH4COmuxxveRdD9zQXGJ29vHLR8SJuK78",
  authDomain: "project2-98ad4.firebaseapp.com",
  projectId: "project2-98ad4",
  storageBucket: "project2-98ad4.firebasestorage.app",
  messagingSenderId: "807037680757",
  appId: "1:807037680757:web:0b2444f98b6de11cde8c3c",
  measurementId: "G-PJYML82PZW"
};

const appAuth = firebase.initializeApp(firebaseConfigAuth, "authApp");
const auth = appAuth.auth();
auth.setPersistence(firebase.auth.Auth.Persistence.NONE);

const googleProvider = new firebase.auth.GoogleAuthProvider();
googleProvider.setCustomParameters({ prompt: 'select_account' });

// ================== PROJECT 2: DATA (REALTIME CẢM BIẾN) - project-2-health ==================
const firebaseConfigData = {
  apiKey: "AIzaSyD3_MWJ-A5wkar9UdDEjo0EuTTmmjxs-vo",
  databaseURL: "https://project-2-health-default-rtdb.asia-southeast1.firebasedatabase.app"
};

const appData = firebase.initializeApp(firebaseConfigData, "dataApp");
const db = appData.database();

// ================== LƯU & XÓA SỐ ĐIỆN THOẠI (dùng project auth) ==================
document.getElementById("phone-form").addEventListener("submit", function(e) {
  e.preventDefault();
  const phone = document.getElementById("phone-input").value.trim();
  const status = document.getElementById("status");

  if (!phone) {
    status.textContent = "Vui lòng nhập số điện thoại!";
    status.style.color = "yellow";
    return;
  }

  db.ref("user/phone/sdt").set(phone)
    .then(() => {
      status.textContent = "Lưu thành công!";
      status.style.color = "green";
      document.getElementById("phone-input").value = "";
    })
    .catch(err => {
      status.textContent = "Lỗi: " + err.message;
      status.style.color = "red";
    });
});

document.getElementById("delete-btn").addEventListener("click", function() {
  db.ref("user/phone/sdt").remove()
    .then(() => {
      document.getElementById("status").textContent = "Đã xóa số điện thoại!";
      document.getElementById("status").style.color = "orange";
    });
});

// ================== HIỂN THỊ DỮ LIỆU CẢM BIẾN + BIỂU ĐỒ (từ project-2-health) ==================
const el = {
  heartbeat: document.getElementById('heartbeat'),
  steps: document.getElementById('steps'),
  spo2: document.getElementById('spo2')
};

let sensorData = { heartbeat: [], steps: [], spo2: [], labels: [] };

const heartChart = new Chart(document.getElementById('heartChart').getContext('2d'), {
  type: 'line',
  data: { labels: [], datasets: [{ label: 'BPM', data: [], borderColor: '#ff6b6b', backgroundColor: 'rgba(255,107,107,0.2)', fill: true, tension: 0.3 }] },
  options: { responsive: true, scales: { x: { display: false } } }
});

const stepChart = new Chart(document.getElementById('stepChart').getContext('2d'), {
  type: 'line',
  data: { labels: [], datasets: [{ label: 'Steps', data: [], borderColor: '#4dabf7', backgroundColor: 'rgba(77,171,247,0.2)', fill: true, tension: 0.3 }] },
  options: { responsive: true, scales: { x: { display: false } } }
});

const spo2Chart = new Chart(document.getElementById('spo2Chart').getContext('2d'), {
  type: 'line',
  data: { labels: [], datasets: [{ label: 'SpO₂ (%)', data: [], borderColor: '#51cf66', backgroundColor: 'rgba(81,207,102,0.2)', fill: true, tension: 0.3 }] },
  options: { responsive: true, scales: { x: { display: false } } }
});

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
  heartChart.update('quiet');

  stepChart.data.labels = sensorData.labels;
  stepChart.data.datasets[0].data = sensorData.steps;
  stepChart.update('quiet');

  spo2Chart.data.labels = sensorData.labels;
  spo2Chart.data.datasets[0].data = sensorData.spo2;
  spo2Chart.update('quiet');
});

// ================== AI DỰ ĐOÁN (DÙNG ONNX) ==================
document.getElementById('ai-predict-btn')?.addEventListener('click', async () => {
  const resultEl = document.getElementById('ai-result-full');

  if (!onnxSession || !aiScaler) {
    resultEl.textContent = "AI đang tải, vui lòng đợi...";
    resultEl.style.color = "orange";
    return;
  }

  const bpm = parseFloat(document.getElementById('ai-bpm').value) || 0;
  const spo2 = parseFloat(document.getElementById('ai-spo2').value) || 0;

  if (bpm < 30 || bpm > 200 || spo2 < 70 || spo2 > 100) {
    resultEl.textContent = "Dữ liệu không hợp lệ!";
    resultEl.style.color = "white";
    resultEl.style.background = "#e74c3c";
    resultEl.style.padding = "16px 24px";
    resultEl.style.borderRadius = "12px";
    return;
  }

  const input = new Float32Array(2);
  input[0] = (bpm - aiScaler.mean[0]) / aiScaler.scale[0];
  input[1] = (spo2 - aiScaler.mean[1]) / aiScaler.scale[1];

  const feeds = { input: new ort.Tensor('float32', input, [1, 2]) };
  const results = await onnxSession.run(feeds);
  const outputTensor = results[Object.keys(results)[0]];
  const predIndex = outputTensor.data.indexOf(Math.max(...outputTensor.data));
  const label = aiLabels[predIndex];
  
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

  resultEl.style.background = colors[label] || '#2c3e50';
  resultEl.style.color = 'white';
  resultEl.style.padding = '16px 24px';
  resultEl.style.borderRadius = '12px';
  resultEl.style.fontWeight = 'bold';
  resultEl.style.textAlign = 'center';
  resultEl.style.boxShadow = `0 0 25px ${colors[label] || '#2c3e50'}99`;
  resultEl.style.transition = 'all 0.4s ease';
});

// ================== ĐĂNG NHẬP & ĐĂNG KÝ EMAIL + GOOGLE (CHỈ HIỆN WELCOME) ==================
// (dùng project auth - project2-98ad4)

// Chuyển form
document.getElementById("show-register")?.addEventListener("click", e => {
  e.preventDefault();
  document.getElementById("login-form").style.display = "none";
  document.getElementById("register-form").style.display = "block";
});
document.getElementById("show-login")?.addEventListener("click", e => {
  e.preventDefault();
  document.getElementById("register-form").style.display = "none";
  document.getElementById("login-form").style.display = "block";
});

// ĐĂNG KÝ EMAIL
document.getElementById("register-btn")?.addEventListener("click", () => {
  const email = document.getElementById("email-register").value.trim();
  const pass  = document.getElementById("password-register").value;

  if (!email || !pass) return alert("Vui lòng nhập đầy đủ!");
  if (pass.length < 6) return alert("Mật khẩu phải ≥ 6 ký tự!");

  auth.createUserWithEmailAndPassword(email, pass)
    .then(() => {
      alert("Đăng ký thành công!");
      document.getElementById("login-section").style.display = "none";
      showSection('welcome');
    })
    .catch(err => alert("Lỗi đăng ký: " + err.message));
});

// ĐĂNG NHẬP EMAIL
document.getElementById("email-signin-btn")?.addEventListener("click", () => {
  const email = document.getElementById("email-login").value.trim();
  const pass  = document.getElementById("password-login").value;

  if (!email || !pass) return alert("Vui lòng nhập email và mật khẩu!");

  auth.signInWithEmailAndPassword(email, pass)
    .then(() => {
      document.getElementById("login-section").style.display = "none";
      showSection('welcome');
    })
    .catch(err => alert("Sai email hoặc mật khẩu!\n" + err.message));
});

// ĐĂNG NHẬP GOOGLE
document.getElementById("login-form").addEventListener("submit", function(e) {
  e.preventDefault();
  const btn = document.querySelector(".btn-login");
  const old = btn.innerHTML;
  btn.disabled = true;
  btn.innerHTML = `<i class="fas fa-spinner fa-spin"></i> Đang mở Google...`;

  auth.signInWithPopup(googleProvider)
    .then(() => {
      document.getElementById("login-section").style.display = "none";
      showSection('welcome');
    })
    .catch(err => {
      btn.disabled = false;
      btn.innerHTML = old;
      alert("Lỗi Google login: " + err.message);
    });
});

// Khi reload trang → chỉ hiện welcome
auth.onAuthStateChanged(user => {
  if (user) {
    document.getElementById("login-section").style.display = "none";
    showSection('welcome');
  }
});

// ================== LOG OUT (SỬA ĐỂ HOÀN HẢO 100%) ==================
function logout() {
  document.getElementById("logo-menu").classList.add("hidden"); // Đóng menu

  // Logout chính xác từ appAuth
  auth.signOut().then(() => {
    // Reset nút Google
    const googleBtn = document.querySelector('#login-form button[type="submit"]');
    if (googleBtn) {
      googleBtn.disabled = false;
      googleBtn.innerHTML = '<i class="fab fa-google" style="margin-right:12px;"></i> Đăng nhập bằng Google';
    }

    // Reset nút Email
    const emailBtn = document.getElementById('email-signin-btn');
    if (emailBtn) {
      emailBtn.disabled = false;
      emailBtn.innerHTML = 'Đăng nhập bằng Email';
    }

    // Hiện lại màn hình login
    document.getElementById("login-section").style.display = "flex";

    // Ẩn hết dashboard
    document.querySelectorAll(".section").forEach(sec => sec.classList.add("hidden"));

    // Xóa value input
    const emailLogin = document.getElementById('email-login');
    const passLogin = document.getElementById('password-login');
    if (emailLogin) emailLogin.value = '';
    if (passLogin) passLogin.value = '';

    console.log("Đăng xuất thành công! Quay lại màn hình login.");

  }).catch(err => {
    alert("Lỗi đăng xuất: " + err.message);
  });
}