import { initializeApp } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-app.js";
import { getDatabase, ref, onValue, set,onChildChanged,onChildAdded} from "https://www.gstatic.com/firebasejs/10.7.1/firebase-database.js";

// Your web app's Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyBg-VCiyYgtsqYAViFtxjD6lQmjLHSFdi8",
  authDomain: "farmself-1beda.firebaseapp.com",
  databaseURL: "https://farmself-1beda-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "farmself-1beda",
  storageBucket: "farmself-1beda.appspot.com",
  messagingSenderId: "170221420258",
  appId: "1:170221420258:web:65270dbb4b395b2388e8bb",
  measurementId: "G-VX41Q9X828"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

var HumidityElement = document.getElementById("humidity");
var temperatureElement = document.getElementById("temperature");
var pumpButton = document.getElementById("pumpButton");
var lightButton = document.getElementById("lightButton");
//Read database
const dataRef = ref(database, "Data");
onValue(dataRef, (snapshot) => {
  const data = snapshot.val();
  HumidityElement.innerText = `${data.Humidity}`;
  temperatureElement.innerText = `${data.Temperature}`;
  // Set button text based on userStatus value
});

const usersRef = ref(database, "users");
onValue(usersRef, (snapshot) => {
  const data = snapshot.val();
  console.log(data.PumpStatus.value)
  pumpButton.innerText = `${data.PumpStatus.value}` === "0" ? "Pump Off" : "Pump On";
  lightButton.innerText = `${data.LightStatus.value}` === "0" ? "Light Off" : "Light On";
});
//Pump
pumpButton.addEventListener("click", () => {
  PumpButtonClicked();
});
function PumpButtonClicked() {
  // Read the current value
  const currentValue = pumpButton.innerText;
  // Toggle the value
  const newValue = currentValue === "Pump On" ? "Pump Off" : "Pump On";
  // Update the button attribute
  pumpButton.setAttribute("data-value", newValue);
  // Update the button text
  pumpButton.innerText = newValue;
  // Update the value in the database
  set_pumpStatus(newValue);
}
function set_pumpStatus(newValue) {
  const PumpvalueToSet = newValue === "Pump On" ? "1" : "0";
  
  set(ref(database, 'users/PumpStatus'), {
    value: PumpvalueToSet
  })
    .then(() => {
      console.log("Pump Status saved successfully!");
    })
    .catch((error) => {
      console.error("The write failed...", error);
    });
}
//Light
lightButton.addEventListener("click", () => {
  lightButtonClicked();
});
function lightButtonClicked() {
  // Read the current value
  const currentValue = lightButton.innerText;
  // Toggle the value
  const newValue = currentValue === "Light On" ? "Light Off" : "Light On";
  // Update the button attribute
  lightButton.setAttribute("data-value", newValue);
  // Update the button text
  lightButton.innerText = newValue;
  // Update the value in the database
  set_lightStatus(newValue);
}
function set_lightStatus(newValue) {
  const LightvalueToSet = newValue === "Light On" ? "1" : "0";
  set(ref(database, 'users/LightStatus'), {
    value: LightvalueToSet
  })
    .then(() => {
      console.log("Light Status saved successfully!");
    })
    .catch((error) => {
      console.error("The write failed...", error);
    });
}