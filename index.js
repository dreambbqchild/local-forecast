const fs = require('fs');
const express = require('express');
const app = express();
const port = 3500;
const timeZone = 'America/Chicago';
const timeStringOptions = { timeZone: timeZone, hour: '2-digit', minute:'2-digit' };

const findNearest = (location) => {
    let max = 0xFFFFFFFF;
    let result = -1;
    for(let i = 0; i < 4; i++) {
        if(location['grid'][i].distance < max) {
            max = location['grid'][i].distance;
            result = i;
        }
    }

    return result;
}

const addHoursToDate = (date, hours) => new Date(date.getTime() + hours * 60 * 60 * 1000);
const prettyDate = (date) => date.toLocaleDateString('en-US', { timeZone });
const prettyTime = (date) => date.toLocaleTimeString('en-US', { timeZone });
const headerRow = (content) => `        <tr>
                <td colspan="11" style="text-align: center;">${content}</td>
            </tr>`;
const tccEmoji = (itcc, lightning, precipTypes, precipRate) =>
{   
    if(!precipRate && lightning)
        return '🌩';
    else if(precipRate && lightning)
        return '⛈';
    else if(precipTypes.length) {
        return precipTypes.map(type => {
            switch(type){
                case 'rain': return '🌧';
                case 'snow': return '🌨';
                case 'freezing rain': return '🧊';
                default: ''
            }
        }).join('');
    }
    else if(itcc < 5)
        return '🌞';
    else if(itcc < 33)
        return '☀';
    else if(itcc < 66)
        return '🌤';
    else if(itcc < 95)
        return '⛅';
    return '☁';
}

const tableBody = (baseDate, hrrr) => 
{
    return Object.entries(hrrr.locations).map(arr =>
    {
        const [key, value] = arr;
        let result = headerRow(key);
        const index = findNearest(value);
        
        for(let i = 0; i < value.temperature.length; i++)
        {
            const newDate = addHoursToDate(baseDate, i);

            if(newDate < addHoursToDate(new Date(),  -1))
                continue;

            if(newDate.getHours() === 0)
                result += headerRow(prettyDate(newDate));

            const time = `${newDate.toLocaleTimeString('en-US', timeStringOptions)}`;
            const temperature = `🌡${parseInt(value.temperature[i][index])}ºF`;
            const dewpoint = `💧${parseInt(value.dewpoint[i][index])}ºF`;
            const rate = value.precip[i][index].rate;
            const pressure = `${value.pressure[i][index].toFixed(2)}`;
            const lightning = parseInt(Math.floor(value.lightning[i][index]));
            const itcc = parseInt(value.totalCloudCover[i][index]);
            const tcc = `${tccEmoji(itcc, lightning, value.precip[i][index].types, rate)} ${itcc}%`;
            const visibility = `${parseInt(value.vis[i][index])}`;
            const windDirection = `${parseInt(value.wind[i][index].dir)}`;
            const windSpeed = `${parseInt(value.wind[i][index].speed)}`;
            const windGust = `${parseInt(value.wind[i][index].gust)}`;

            result += `           <tr>
                    <td>${time}</td>
                    <td>${temperature} ${dewpoint}</td>
                    <td>${pressure}</td>
                    <td>${tcc}</td>
                    <td>${rate.toFixed(5)}</td>
                    <td>${visibility}</td>
                    <td>${windDirection} @ ${windSpeed} G ${windGust}</td>
                </tr>`;
        }
        return result + headerRow("<hr/>");
    }).join('');
}

const renderForHour = (hour) => {
    const hrrr = JSON.parse(fs.readFileSync(`./forecasts/hrrr-${hour}.json`, 'utf8'));
    const baseDate = new Date(hrrr.date);

    return `<!DOCTYPE html>
<html>
    <head>
        <title>${prettyDate(baseDate)} ${prettyTime(baseDate)} Forecast</title>
        <style>
            html, body { font-family:'Segoe UI Emoji'; font-size: 1.5em; background: black; color: white;}
            table {width: 100vw;}
        </style>
    </head>
    <body>
        <table>
            ${tableBody(baseDate, hrrr)}
        </table>
    </body>
</html>`
}

app.get('/', (req, res) => {
    const hour = fs.readFileSync('./forecasts/lastRun', 'utf8');
    res.send(renderForHour(hour));
});

app.get('/:hour', (req, res) => {
    res.send(renderForHour(req.params.hour));
});

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
});