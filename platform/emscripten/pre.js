Module.canvas = document.getElementById("canvas");
if(Module.canvas == undefined) Module.canvas = document.getElementByTagName("canvas");
if(Module.canvas == undefined) alert("No canvas?");
Module.canvas.addEventListener("contextmenu", function() {event.preventDefault()});
Module.canvas.addEventListener("dragover", function() {event.preventDefault()});
Module.canvas.addEventListener("drop", function() {meg4_openfile(event)});
var tag = document.createElement("INPUT");
tag.setAttribute("id", "meg4_upload");
tag.setAttribute("type", "file");
tag.setAttribute("style", "display:none");
tag.addEventListener("change", function() {
    inp = document.getElementById("meg4_upload");
    if(inp.files.length>0) {
        var reader = new FileReader();
        reader.onloadend = function() { meg4_execute(inp.files[0].toString(), new Uint8Array(reader.result)); };
        reader.readAsArrayBuffer(inp.files[0]);
    }
});
document.body.appendChild(tag);
tag = document.createElement("A");
tag.setAttribute("id", "meg4_download");
tag.setAttribute("style", "display:none");
tag.setAttribute("download", "");
document.body.appendChild(tag);
function meg4_execute(name, data) {
    var base = name.split('/').pop();
    var file = new TextEncoder("utf-8").encode(base == undefined ? name : base);
    if(data != undefined && data.length > 0) {
        var meg4 = Module.cwrap("meg4_insert", "number", [ "number", "number", "number" ]);
        const fn = Module._malloc(file.length);
        const buf = Module._malloc(data.length);
        Module.HEAPU8.set(file, fn);
        Module.HEAPU8.set(data, buf);
        if(meg4 != undefined) meg4(fn, buf, data.length);
        Module._free(buf);
        Module._free(fn);
    }
}
function meg4_openfile(e) {
    e.stopPropagation();
    e.preventDefault();
    var floppy = e.dataTransfer.getData("url");
    if(floppy == undefined || floppy == "")
        floppy = e.dataTransfer.getData("src");
    if(floppy == undefined || floppy == "")
        floppy = new DOMParser().parseFromString(e.dataTransfer.getData('text/html'), "text/html").querySelector('img').src;
    if(floppy != undefined && floppy != "") {
      fetch(floppy).then((response) => {
        if(response.ok) response.arrayBuffer().then((buf) => { meg4_execute(floppy, new Uint8Array(buf)); });
      })
      .catch((error) => {
        var reader = new FileReader();
        reader.onloadend = function() { meg4_execute(floppy, new Uint8Array(reader.result)); };
        reader.readAsArrayBuffer(floppy);
      });
    }
}
function meg4_savefile(fn, fnlen, buf, len) {
    const fview = new Uint8Array(Module.HEAPU8.buffer,fn,fnlen);
    const bview = new Uint8Array(Module.HEAPU8.buffer,buf,len);
    var name = new TextDecoder("utf-8").decode(fview);
    var blob = new Blob([bview], { type: "application/octet-stream" });
    var url = window.URL.createObjectURL(blob);
    var a = document.getElementById('meg4_download');
    name = prompt("Save As", name);
    if(name) {
        a.setAttribute('href',url);
        a.setAttribute('download',name);
        a.click();
    }
    window.URL.revokeObjectURL(url);
}
