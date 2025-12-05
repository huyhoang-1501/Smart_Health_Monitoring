### 1. System Overview
```
graph TD
    A[ESP32 + MAX30100 + MPU6050] 
    -->|Measure HR, SpO₂, Steps| B[Firebase Realtime DB<br>project-2-health]

    B --> C[Web Dashboard<br>JavaScript + Chart.js]
    C --> D[AI ONNX Model<br>Health Prediction]
    D --> E[Display results + color warning]

    B --> F[A7682S SIM Module]
    F -->|Danger > 90 seconds| G[Auto Call<br>Saved Number]

    H[User saves phone number<br>via Web or Keypad] --> B

    style A fill:#ff6b6b,stroke:#333,color:#fff
    style C fill:#4dabf7,stroke:#333,color:#fff
    style D fill:#51cf66,stroke:#333,color:#fff
    style G fill:#e74c3c,stroke:#fff,color:#fff
```

### 2. Sensor Data Flow
```
graph TD
    M1["MAX30100<br>Heart Rate + SpO₂"] --> U1["pox.update() every loop"]
    M2["MPU6050<br>Accelerometer"] --> U2["Step counting<br>threshold + debounce"]

    U1 & U2 --> T["Every 60 seconds<br>Get average values"]
    T --> F["Send to Firebase<br>/parameter/..."]
    F --> W["Web reads & Chart.js<br>Real-time update"]

    style M1 fill:#ff6b6b,color:#fff
    style M2 fill:#ff9f43,color:#fff
    style W fill:#4dabf7,color:#fff
```


### 3. Warning & Emergency Call Mechanism
```
graph TD
    Start["Continuous monitoring"] --> HR{"HR <50 or >100?"}
    HR -->|Yes| W1["bpm_warning +1"]
    HR -->|No| W1

    Step{"Steps ≤80 in 1 min?"} --> W2["step_warning +1"]

    W1 & W2 --> Trigger{"Warning triggered?"}
    Trigger -->|Yes| Alert["Buzzer + LED ON"]
    Alert --> Timer["90-second countdown"]
    Timer --> TimeUp{"90s passed?"}
    TimeUp -->|Yes| Call["EMERGENCY CALL<br>ATD + number"]
    TimeUp -->|No| Wait["Wait for button A"]
    Wait -->|Press A| Off["Cancel alert"]

    style Call fill:#e74c3c,stroke:#fff,color:#fff
    style Alert fill:#f39c12,color:#333

### 4. AI Health Prediction Flow
```
graph LR
    A["Web receives real-time data"] --> B["Auto fill BPM & SpO₂"]
    B --> C["Click Predict or autoPredict()"]
    C --> D["Normalize: z-score"]
    D --> E["onnxSession.run()"]
    E --> F["Argmax → class"]
    F --> G["Show result + color"]

    style G fill:#2ecc71,stroke:#333,color:#fff
    style C fill:#3498db,color:#fff
```


### 5. Phone Number Management
```
graph TD
    subgraph Web["Web Interface"]
        W1["Enter phone number"] --> W2["Save to Firebase"]
    end

    subgraph ESP32["ESP32 + Keypad"]
        K1["Press *"] --> K2["Enter number mode"]
        K2 --> K3["Press digits"]
        K3 --> K4["Press D → Save"]
        K4 --> K5["Upload to Firebase"]
        K6["Press C"] --> K7["Call now"]
    end

    W2 & K5 --> FB["Firebase<br>Saved Number"]
    FB --> K7

    style FB fill:#f1c40f,stroke:#333
    style K7 fill:#e74c3c,color:#fff
```
