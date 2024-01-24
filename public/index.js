import { html } from 'html-express-js';
const clipBorderWidth = 32;
const widthForClipping = 1060 + clipBorderWidth * 2;
const heightForClipping = 1100 + clipBorderWidth * 2;
export const view = (data, state) => html`
  <!DOCTYPE html>
  <html lang="en">
    <head>
      <title>Wx Map</title>
      <script>
        let map = null;

        function initMap() {
          map = new google.maps.Map(document.getElementById("map"), {
            center: { lat: 44.90100491755502, lng: -93.24577392578125 },
            zoom: 11,
            disableDefaultUI: true
          });
        }

        function renderConstLine()
        {
          const bounds = map.getBounds();
          const ne = bounds.getNorthEast();
          const sw = bounds.getSouthWest();
          const coords = {
            topLat: ne.lat(),
            bottomLat: sw.lat(),
            leftLon: sw.lng(),
            rightLon: ne.lng(),
          };

          document.getElementById("consts").textContent = \`"coords": \${JSON.stringify(coords, null, '\t')}\`;
          
          document.getElementById("plotPoints").textContent = sw.lat() + ',' + ne.lng() + ',#FF0000,marker,topLeft\n' + ne.lat() + ',' + sw.lng() + ',#FF0000,marker,bottomRight';
        }

      </script>
    </head>
    <body>
      <div id="map" style="width: ${widthForClipping}px; height:${heightForClipping}px"></div>
      <button onclick="renderConstLine()">Render</button>
      <div id="consts"></div>
      <pre id="plotPoints"></pre>
    </body>
    <script
      src="https://maps.googleapis.com/maps/api/js?key=${process.env.GOOGLE_MAPS_API_KEY}&callback=initMap&v=weekly"
      defer
    ></script>
  </html>
`;