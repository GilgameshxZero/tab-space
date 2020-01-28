// Send XHR for data.
function sendXhr(url, onResponse, body) {
  const xhr = new XMLHttpRequest();
  xhr.open(`POST`, url, true);
  xhr.onreadystatechange = () => {
    if (xhr.readyState !== 4) return;
    if (onResponse != null) {
      if (xhr.status !== 200) onResponse(null);
      else onResponse(xhr.responseText);
    }
  };
  xhr.send(body);
};

window.addEventListener(`load`, () => {
  // Get the tab ID from the URL.
  const idSplit = window.location.href.split(`/`);
  const id = idSplit[idSplit.length - 1].split(`?`)[0];

  const videoImgElem = document.querySelector(`.video`);
  videoImgElem.src = `/stream/${id}`;

  // Keep focus on video.
  videoImgElem.addEventListener(`blur`, (event) => {
    event.target.focus();
  });

  // Add relevant event handlers on video.
  window.addEventListener(`mousedown`, (event) => {
    const videoImgElemBounds = videoImgElem.getBoundingClientRect();
    const width = videoImgElemBounds.right - videoImgElemBounds.left;
    const height = videoImgElemBounds.bottom - videoImgElemBounds.top;
    const x = event.clientX - videoImgElemBounds.left;
    const y = event.clientY - videoImgElemBounds.top;
    sendXhr(`/action/${id}/mouse`, null, JSON.stringify({
      "direction": "down",
      "x": x / width,
      "y": y / height,
    }));
  });
  window.addEventListener(`mouseup`, (event) => {
    const videoImgElemBounds = videoImgElem.getBoundingClientRect();
    const width = videoImgElemBounds.right - videoImgElemBounds.left;
    const height = videoImgElemBounds.bottom - videoImgElemBounds.top;
    const x = event.clientX - videoImgElemBounds.left;
    const y = event.clientY - videoImgElemBounds.top;
    sendXhr(`/action/${id}/mouse`, null, JSON.stringify({
      "direction": "up",
      "x": x / width,
      "y": y / height,
    }));
  });
  window.addEventListener(`keydown`, (event) => {
    sendXhr(`/action/${id}/key`, null, JSON.stringify({
      "key": event.key,
    }));
  });
});