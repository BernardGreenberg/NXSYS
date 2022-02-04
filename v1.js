function mmhandler(e) {
    LastX = e.pageX;
    LastY = e.pageY;
}

window.onmousemove = mmhandler
LAST_ELT= 0
function defenter(elt) {

   tag = elt.getAttribute("dref").substr(1)
   pu = document.getElementById(tag);
 LAST_ELT=pu
   pu.style.top = LastY + "px"
   pu.style.left = LastX + "px"
   pu.style.width = "300px"
   pu.style.display="block";
}

function defleave() {
   LAST_ELT.style.display = "none"
}
