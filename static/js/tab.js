// Apply class to body and root if mobile.
function testApplyMobileStyling() {
  // detect if mobile and apply separate styling
  if (/(android|bb\d+|meego).+mobile|avantgo|bada\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|ipad|iris|kindle|Android|Silk|lge |maemo|midp|mmp|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\.(browser|link)|vodafone|wap|windows (ce|phone)|xda|xiino/i.test(navigator.userAgent) ||
    /1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\-(n|u)|c55\/|capi|ccwa|cdm\-|cell|chtm|cldc|cmd\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\-s|devi|dica|dmob|do(c|p)o|ds(12|\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\-|_)|g1 u|g560|gene|gf\-5|g\-mo|go(\.w|od)|gr(ad|un)|haie|hcit|hd\-(m|p|t)|hei\-|hi(pt|ta)|hp( i|ip)|hs\-c|ht(c(\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\-(20|go|ma)|i230|iac( |\-|\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\/)|klon|kpt |kwc\-|kyo(c|k)|le(no|xi)|lg( g|\/(k|l|u)|50|54|\-[a-w])|libw|lynx|m1\-w|m3ga|m50\/|ma(te|ui|xo)|mc(01|21|ca)|m\-cr|me(rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\-2|po(ck|rt|se)|prox|psio|pt\-g|qa\-a|qc(07|12|21|32|60|\-[2-7]|i\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\-|oo|p\-)|sdk\/|se(c(\-|0|1)|47|mc|nd|ri)|sgh\-|shar|sie(\-|m)|sk\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\-|v\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\-|tdg\-|tel(i|m)|tim\-|t\-mo|to(pl|sh)|ts(70|m\-|m3|m5)|tx\-9|up(\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas\-|your|zeto|zte\-/i.test(navigator.userAgent.substr(0, 4))) {
    document.body.classList.add(`mobile`);
    document.documentElement.classList.add(`mobile`);
  }
}

// Send XHR for data.
function sendXhr(method, url, onResponse, body) {
  const xhr = new XMLHttpRequest();
  xhr.open(method, url, true);
  xhr.onreadystatechange = () => {
    if (xhr.readyState !== 4) return;
    if (onResponse != null) {
      if (xhr.status !== 200) onResponse(null);
      else onResponse(xhr.responseText);
    }
  };
  xhr.send(body);
};

function sendMouseAction(state, event, type, direction) {
  // If event target is an input element, send it to the input element instead of the tab.
  if (event.target.tagName === `INPUT`)
    return;
  if (event.target.classList.contains(`button`) || event.target.classList.contains(`callout`))
    return;

  const videoImgElemBounds = state.videoImg.getBoundingClientRect();
  const width = videoImgElemBounds.right - videoImgElemBounds.left;
  const height = videoImgElemBounds.bottom - videoImgElemBounds.top;
  let x = (event.clientX - videoImgElemBounds.left) / width;
  let y = (event.clientY - videoImgElemBounds.top) / height;

  let outOfBounds = false;

  // If mouse L button is held down, then clamp x and y.
  if (state.mouseLDown) {
    x = Math.min(Math.max(x, 0), 1);
    y = Math.min(Math.max(y, 0), 1);
  } else {
    // If out of bounds and not a drag event, don't send the event.
    if (state.mouseLDown = false && (x < 0 || x > 1 || y < 0 || y > 1)) {
      outOfBounds = true;
    }
  }

  if (type === `lclick`) {
    if (direction === `down`) {
      state.mouseLDown = true;
    } else if (direction === `up`) {
      state.mouseLDown = false;
    }
  }

  if (outOfBounds) return;

  x = x * state.sourceWidth - 0.01;
  y = y * state.sourceHeight - 0.01;

  const eventJsonString = JSON.stringify({
    "type": type,
    "direction": direction,
    "x": x,
    "y": y,
    "wheelDeltaX": -event.deltaX * state.WHEEL_EVENT_MULTIPLIER,
    "wheelDeltaY": -event.deltaY * state.WHEEL_EVENT_MULTIPLIER,
  });
  sendXhr(`POST`, `/action/${state.id}/mouse`, null, eventJsonString);
}

function handleKeyAction(state, event, direction) {
  // If event target is an input element or button, send it to the input element instead of the tab.
  if (event.target.tagName === `INPUT`)
    return;
  if (event.target.classList.contains(`button`) || event.target.classList.contains(`callout`))
    return;

  event.preventDefault();
  const eventJsonString = JSON.stringify({
    "direction": direction,
    "key": event.key,
  });
  sendXhr(`POST`, `/action/${state.id}/key`, null, eventJsonString);
}

window.addEventListener(`load`, () => {
  testApplyMobileStyling();

  // Global state for client.
  const state = {
    WHEEL_EVENT_MULTIPLIER: 25,

    sourceWidth: 1280,
    sourceHeight: 720,

    mouseLDown: false,
    id: ``,

    share: document.querySelector(`.share`),
    resolution: document.querySelector(`.resolution`),
    resolutionWidth: document.querySelector(`.resolution .width`),
    resolutionHeight: document.querySelector(`.resolution .height`),
    profile: document.querySelector(`.profile`),

    videoImg: document.querySelector(`.video`),

    lastFetchedUrl: ``,
  };

  // Get the tab ID from the URL.
  const idSplit = window.location.href.split(`/`);
  state.id = idSplit[idSplit.length - 1].split(`?`)[0];

  state.videoImg.src = `/stream/${state.id}`;

  // TODO: Use Websocket. Periodically pull URL from backend.
  const pullUrl = () => {
    sendXhr(`GET`, `/tab/${state.id}/url`, (responseText) => {
      if (responseText !== state.lastFetchedUrl) {
        state.lastFetchedUrl = responseText;
        document.querySelector(`.controls>.url>input`).value = responseText;
      }
      setTimeout(pullUrl, 500);
    });
  };
  pullUrl();

  // Controls panel left and URL.
  document.querySelector(`.back`).addEventListener(`click`, () => {
    sendXhr(`POST`, `/control/${state.id}/back`);
  });
  document.querySelector(`.forward`).addEventListener(`click`, () => {
    sendXhr(`POST`, `/control/${state.id}/forward`);
  });
  document.querySelector(`.reload`).addEventListener(`click`, () => {
    sendXhr(`POST`, `/control/${state.id}/reload`);
  });
  document.querySelector(`.url>input`).addEventListener(`keyup`, (event) => {
    if (event.keyCode == 13) {
      sendXhr(`POST`, `/tab/${state.id}/url`, null, event.target.value);
    }
  });

  // Callouts.
  state.share.querySelector(`.icon-wrapper`).addEventListener(`click`, (event) => {
    state.share.classList.toggle(`opened`);
    state.resolution.classList.remove(`opened`);
    state.profile.classList.remove(`opened`);
  });
  state.resolution.querySelector(`.icon-wrapper`).addEventListener(`click`, () => {
    state.share.classList.remove(`opened`);
    state.resolution.classList.toggle(`opened`);
    state.profile.classList.remove(`opened`);
  });
  state.profile.querySelector(`.icon-wrapper`).addEventListener(`click`, () => {
    state.share.classList.remove(`opened`);
    state.resolution.classList.remove(`opened`);
    state.profile.classList.toggle(`opened`);
  });

  // Share controls.
  const shareLink = state.share.querySelector(`.callout>input.link`);
  shareLink.value = window.location;
  shareLink.addEventListener(`click`, (event) => {
    event.stopPropagation();
    shareLink.select();
    shareLink.setSelectionRange(0, 10000); // For mobile devices.
    document.execCommand(`copy`);
  });
  const shareOnly = state.share.querySelector(`.callout>.share-only`);
  sendXhr(`GET`, `/control/${state.id}/share/only`, (responseText) => {
    console.log(responseText === `true`);
    shareOnly.querySelector(`input`).checked = responseText === `true`;
  });
  shareOnly.addEventListener(`click`, (event) => {
    event.stopPropagation();
    sendXhr(`POST`, `/control/${state.id}/share/only`, null, event.target.checked ? `true` : `false`);
  });
  const shareOnlyUsernames = state.share.querySelector(`.callout>input.usernames`);
  sendXhr(`GET`, `/control/${state.id}/share/only/usernames`, (responseText) => {
    shareOnlyUsernames.value = responseText;
  });
  shareOnlyUsernames.addEventListener(`click`, (event) => {
    event.stopPropagation();
  });
  shareOnlyUsernames.addEventListener(`blur`, (event) => {
    event.stopPropagation();
    sendXhr(`POST`, `/control/${state.id}/share/only/usernames`, null, JSON.stringify({
      "usernames": shareOnlyUsernames.value.split(` `).filter((x) => x != ``),
    }));
  });

  // Resolution controls.
  const cloneMyResolution = () => {
    const videoImgElemBounds = document.querySelector(`.video-wrapper`).getBoundingClientRect();
    state.resolutionWidth.value = parseInt(videoImgElemBounds.right - videoImgElemBounds.left);
    state.resolutionHeight.value = parseInt(videoImgElemBounds.bottom - videoImgElemBounds.top);
  };
  const updateBackendResolution = () => {
    state.sourceWidth = state.resolutionWidth.value;
    state.sourceHeight = state.resolutionHeight.value;
    sendXhr(`POST`, `/control/${state.id}/resolution`, null, JSON.stringify({
      "width": state.sourceWidth,
      "height": state.sourceHeight,
    }));
  };
  const getBackendResolution = () => {
    sendXhr(`GET`, `/control/${state.id}/resolution`, (responseText) => {
      const dimensions = responseText.split(` `);
      state.resolutionWidth.value = parseInt(dimensions[0]);
      state.resolutionHeight.value = parseInt(dimensions[1]);
    });
  };
  // Maximize resolution if first listener.
  sendXhr(`GET`, `/stream/${state.id}/count`, (responseText) => {
    if (parseInt(responseText) == 1) {
      cloneMyResolution();
      updateBackendResolution();
    } else {
      getBackendResolution();
    }
  });
  state.resolution.querySelector(`.callout .clone`).addEventListener(`click`, (event) => {
    event.stopPropagation();
    cloneMyResolution();
  });
  state.resolution.querySelectorAll(`.callout input`).forEach((node) => node.addEventListener(`click`, (event) => {
    event.stopPropagation();
  }));
  state.resolution.querySelector(`.callout .set`).addEventListener(`click`, (event) => {
    event.stopPropagation();
    updateBackendResolution();
  });

  // Profile controls.
  const saltHashPassword = (password) => {
    return md5(`${password}emt!`);
  };
  state.profile.querySelector(`.callout input.username`).addEventListener(`click`, (event) => {
    event.stopPropagation();
  });
  state.profile.querySelector(`.callout input.password`).addEventListener(`click`, (event) => {
    event.stopPropagation();
  });
  state.profile.querySelector(`.callout .login`).addEventListener(`click`, (event) => {
    event.stopPropagation();
  });
  state.profile.querySelector(`.callout .create`).addEventListener(`click`, (event) => {
    event.stopPropagation();
  });
  state.profile.querySelector(`.callout .logout`).addEventListener(`click`, (event) => {
    event.stopPropagation();
  });

  // Use cookie to see if we're already logged in, and enable flags accordingly.

  // Add relevant event handlers on video.
  window.addEventListener(`mousedown`, (event) => {
    sendMouseAction(state, event, `lclick`, `down`);
  });
  window.addEventListener(`mouseup`, (event) => {
    sendMouseAction(state, event, `lclick`, `up`);
  });
  window.addEventListener(`mousemove`, (event) => {
    sendMouseAction(state, event, `move`, ``);
  });
  window.addEventListener(`wheel`, (event) => {
    sendMouseAction(state, event, `wheel`, ``);
  });
  window.addEventListener(`keydown`, (event) => {
    handleKeyAction(state, event, `down`);
  });
  window.addEventListener(`keyup`, (event) => {
    handleKeyAction(state, event, `up`);
  });
});