// Send XHR for data.
function requestData(url, onResponse) {
  const xhr = new XMLHttpRequest();
  xhr.open(`GET`, url, true);
  xhr.onreadystatechange = () => {
    if (xhr.readyState !== 4) return;
    if (xhr.status !== 200) onResponse(null);
    else onResponse(xhr.responseText);
  };
  xhr.send();
};

window.addEventListener(`load`, () => {
  // Get the tab ID from the URL.
  const tabIdSplit = window.location.href.split(`/`);
  const tabId = tabIdSplit[tabIdSplit.length - 1];
  console.log(`Requesting stream for tab ID`, tabId);

  const imgElem = document.querySelector(`.video img`);
  imgElem.src = `/stream/` + tabId;
});