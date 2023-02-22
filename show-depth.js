import path from 'path';
import fs from 'fs';

const cellColor = (value) => {
    let rgb = [255, 255, 255];
    if(value < 0.1)
        rgb = [255, 255, 255];
    else if(value < 0.25)
        rgb = [200, 200, 200];
    else if(value < 0.5)
        rgb = [175, 175, 175];
    else if(value < 0.75)
        rgb = [150, 150, 150];
    else if(value < 1)
        rgb = [169, 231, 242];
    else if(value < 1.5)
        rgb = [114, 191, 215];
    else if(value < 2)
        rgb = [59, 152, 187];
    else if(value < 2.5)
        rgb = [4, 112, 160];
    else if(value < 3)
        rgb = [0, 70, 176];
    else if(value < 3.5)
        rgb = [39, 100, 188];
    else if(value < 4)
        rgb = [77, 130, 200];
    else if(value < 4.5)
        rgb = [116, 160, 211];
    else if(value < 5)
        rgb = [154, 190, 223];
    else if(value < 5.5)
        rgb = [193, 220, 235];
    else if(value < 6)
        rgb = [201, 173, 219];
    else if(value < 7)
        rgb = [193, 149, 210];
    else if(value < 8)
        rgb = [185, 126, 201];
    else if(value < 9)
        rgb = [176, 102, 193];
    else if(value < 10)
        rgb = [169, 79, 184];
    else if(value < 11)
        rgb = [160, 55, 175];
    else if(value < 12)
        rgb = [135, 10, 71];
    else if(value < 14)
        rgb = [151, 28, 90];
    else if(value < 16)
        rgb = [167, 45, 110];
    else if(value < 18)
        rgb = [182, 63, 129];
    else if(value < 20)
        rgb = [198, 80, 149];
    else if(value < 22)
        rgb = [214, 98, 168];
    else if(value < 24)
        rgb = [235, 167, 183];
    else if(value < 26)
        rgb = [232, 155, 161];
    else if(value < 28)
        rgb = [230, 143, 138];
    else if(value < 30)
        rgb = [227, 130, 116];
    else if(value < 32)
        rgb = [225, 118, 93];
    else if(value < 34)
        rgb = [222, 106, 71];
    else if(value < 36)
        rgb = [220, 129, 80];
    else if(value < 40)
        rgb = [225, 149, 104];
    else if(value < 44)
        rgb = [230, 169, 127];
    else if(value < 48)
        rgb = [236, 189, 151];
    else if(value < 52)
        rgb = [241, 209, 175];
    else if(value < 56)
        rgb = [246, 229, 198];
    else
        rgb = [251, 249, 222];

    return `rgb(${rgb.join(',')});`;
}

function* totalNew(howMany) {
    for(let i = 0; i < howMany; i++)
        yield "<td>Total</td><td>New</td>";
}

let folder = path.join('forecasts', 'hrrr');
if(process.argv.length > 2)
    folder = path.join('forecasts', process.argv[2]);

const now = new Date();
const forecast = JSON.parse(fs.readFileSync(path.join(folder, 'forecast.json')));
const tableData = [];
let tableHtml = '<table><thead><tr><td></td>'
for(const [name, location] of Object.entries(forecast.locations)) {
    if(location.isCity)
        continue;

    tableHtml += `<td colspan="2">${name}</td>`
    tableData.push(location.wx.totalSnow);
}

tableHtml += `</tr><tr><td>Time</td>${[...totalNew(tableData.length)].join('')}`;

tableHtml += '</tr></thead><tbody>';
for(const [index, strDate] of forecast.forecastTimes.entries()) {
    const date = new Date(strDate);
    if(date < now)
        continue;

    if(!date.getHours())
        tableHtml += `<tr><td colspan="${tableData.length * 2 + 1}">${date.toLocaleDateString('en-US', {month: '2-digit', day: '2-digit', weekday: 'long'})}</td></tr>`;

    tableHtml += `<tr><td>${date.toLocaleTimeString('en-US', {hour: '2-digit', minute: '2-digit'})}</td>`;
    for(const totalSnow of tableData){
        const total = totalSnow[index];
        const newSnow = index ? total - totalSnow[index - 1] : total;
        tableHtml += `<td style="background: ${cellColor(total)}">${total.toFixed(2)}</td><td style="background: ${cellColor(total)}">${newSnow.toFixed(2)}</td>`;
    }
    tableHtml += "</tr>";
}

tableHtml += '</tbody></table>';

fs.writeFileSync(path.join('renderings', 'snow-depth.html'), `<!DOCTYPE html>
<html>
    <head>
        <title>Snow Depth</title>
        <style>
            table {text-align: center; font-family: sans-serif; table-layout: fixed; width: 100%; border-collapse: collapse; box-sizing: border-box;}
            thead, [colspan] {font-weight: bold;}
            td {border-right: solid black 1px;}
        </style>
    </head>
    <body>
${tableHtml}
    </body>
</html>`);