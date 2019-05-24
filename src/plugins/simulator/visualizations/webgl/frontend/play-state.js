/**
 * Gray out buttons PLAY/STEP/PAUSE
 */

let buttonSendStep = document.getElementById("sendStep");
let buttonSendAuto = document.getElementById("sendAuto");
let buttonSendPause = document.getElementById("sendPause");

let isAuto = false;

function disable(button) {
    button.setAttribute("disabled", "disabled");
}

function enable(button) {
    button.removeAttribute("disabled");
}

function updateButtonGray() {
    // activated when playing
    let playing = [buttonSendPause];
    // activated when paused
    let pause = [buttonSendStep, buttonSendAuto];

    if (isAuto) {
        playing.map(enable);
        pause.map(disable);
    } else {
        playing.map(disable);
        pause.map(enable);
    }
}

updateButtonGray();