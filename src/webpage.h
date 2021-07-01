#include<Arduino.h>
const char webpage_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <title>Mudskipper Robot Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
        html {
            font-family: Helvetica;
            display: inline-block;
            margin: 0px auto;
            text-align: center;
        }

        h1 {
            color: #0F3376;
            padding: 2vh;
        }

        p {
            font-size: 1.5rem;
        }

        .button {
            display: inline-block;
            background-color: #e7bd3b;
            border: none;
            border-radius: 50%;
            color: white;
            padding: 16px 40px;
            text-decoration: none;
            font-size: 30px;
            margin: 2px;
            cursor: pointer;
        }

        .button2 {
            background-color: #f44242;
        }
        .button3 {
            background-color: #85e967;
            border-radius: 0%;
        }
        .slidecontainer {
  width: 100%; /* Width of the outside container */
}

/* The slider itself */
.slider {
  -webkit-appearance: none;  /* Override default CSS styles */
  appearance: none;
  width: 50%; /* Full-width */
  height: 25px; /* Specified height */
  background: #d3d3d3; /* Grey background */
  outline: none; /* Remove outline */
  opacity: 0.7; /* Set transparency (for mouse-over effects on hover) */
  -webkit-transition: .2s; /* 0.2 seconds transition on hover */
  transition: opacity .2s;
}

/* Mouse-over effects */
.slider:hover {
  opacity: 1; /* Fully shown on mouse-over */
}

/* The slider handle (use -webkit- (Chrome, Opera, Safari, Edge) and -moz- (Firefox) to override default look) */
.slider::-webkit-slider-thumb {
  -webkit-appearance: none; /* Override default look */
  appearance: none;
  width: 25px; /* Set a specific slider handle width */
  height: 25px; /* Slider handle height */
  background: #04AA6D; /* Green background */
  cursor: pointer; /* Cursor on hover */
}

.slider::-moz-range-thumb {
  width: 25px; /* Set a specific slider handle width */
  height: 25px; /* Slider handle height */
  background: #04AA6D; /* Green background */
  cursor: pointer; /* Cursor on hover */
}
    </style>
</head>

<body>
    <h1>Mudskipper Controller</h1>
    </p>
    <p><button class="button" onmousedown="toggleCheckbox('forward');" ontouchstart="toggleCheckbox('forward');">&#8593;</button></a></p> 
    <p><button class="button" onmousedown="toggleCheckbox('left');" ontouchstart="toggleCheckbox('left');">&#8592;</button></a>
        <button class="button button2" onmousedown="toggleCheckbox('stop');" ontouchstart="toggleCheckbox('stop');">&#88;</button></a>
        <button class="button" onmousedown="toggleCheckbox('right');" ontouchstart="toggleCheckbox('right');">&#8594;</button></a></p>
    <p><button class="button" onmousedown="toggleCheckbox('back');" ontouchstart="toggleCheckbox('back');">&#8595;</button></a></p>
    <p><button class="button button3" onmousedown="toggleCheckbox('auto');" ontouchstart="toggleCheckbox('auto');">Zombie Mode</button></a></p>
    <p><div class="slidecontainer">
        Speed:<input type="range" onchange="updateSpeed(this)" min="0" max="255" value="0" class="slider" id="speedSlider">
      </div></p>
    <p>Distance from enemy : 
       <span id="distance"></span>
       <sup class="units">mm</sup>
    </p>
</body>
<script>
    
  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("distance").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/distance", true);
  xhttp.send();
  }, 2000) ;

    function updateSpeed(element){
    var speedValue = document.getElementById("speedSlider").value;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/slider?speedValue="+speedValue, true);
    xhr.send();

    }
    function toggleCheckbox(x) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/" + x, true);
      xhr.send();
    }
   </script>
</html>)rawliteral";