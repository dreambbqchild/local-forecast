const fs = require('fs');
const { getSunrise, getSunset } = require('sunrise-sunset-js');
const timeZone = 'America/Chicago';
const timeStringOptions = { timeZone: timeZone, hour: '2-digit', minute:'2-digit' };

const findNearest = (location) => {
    let max = 0xFFFFFFFF;
    let result = -1;
    for(let i = 0; i < 4; i++) {
        if(location.grid[i].distance < max) {
            max = location.grid[i].distance;
            result = i;
        }
    }

    return result;
}

const findWeightsForAverage = (location) => {
    const max = Math.max(...location.grid.map(c => c.distance)) + 1;
    const weights = [];
    for(let i = 0; i < 4; i++)
        weights.push(-(max - location.grid[i].distance));

    return weights;
}

const dir = (deg, useArrows = true) => {
    const arrows = ["↓", "↙", "←", "↖", "↑", "↗", "→", "↘"];
    const dirs = [" N", "NE", " E", "SE", " S", "SW", " W", "NW"];
    const arr = useArrows ? arrows : dirs;
    return `${arr[Math.trunc(((deg + 22) % 360) / 45)]}`;
}

const addHoursToDate = (date, hours) => new Date(date.getTime() + hours * 60 * 60 * 1000);
const prettyDate = (date) => date.toLocaleDateString('en-US', { timeZone });
const prettyTime = (date) => date.toLocaleTimeString('en-US', { timeZone });
const tccEmoji = (itcc, lightning, precipTypes, precipRate) =>
{   
    if(!precipRate && lightning)
        return '🌩';
    else if(precipRate && lightning)
        return '⛈';
    else if(precipTypes.length) {
        return precipTypes.map(type => {
            switch(type){
                case 'rain': return '🌧️';
                case 'snow': return '🌨️';
                case 'freezing rain': return '🧊';
                default: ''
            }
        }).join('');
    }
    else if(itcc < 5)
        return '🌞';
    else if(itcc < 33)
        return '☀️';
    else if(itcc < 66)
        return '🌤️';
    else if(itcc < 95)
        return '⛅';
    return '☁️';
}

const visEmoji = (vis, coords, date) => {
    if(vis <= 3)
        return '🌫️'
    else if (vis <= 6)
        return '🌁';

    var sunrise = getSunrise(coords.lat, coords.lon, date);
    var sunset = getSunset(coords.lat, coords.lon, date);

    if(sunrise >= date || sunset <= date)
        return '🌃';
    
    return '🏙️';
}

const tableBody = (baseDate, hrrr) => 
{
    return Object.entries(hrrr.locations).map(arr =>
    {
        const [key, value] = arr;
        let result = `---${key}---\n`;
        const index = findNearest(value);
        const weights = findWeightsForAverage(value);
        const weightsSum = weights.reduce((a, b) => a + b, 0);
 
        const weighted = (values, property) => {
            let result = 0;
            for(let i = 0; i < 4; i++) {
                const value = property ? values[i][property] : values[i];
                result += weights[i] * value;
            }
            return result / weightsSum;
        }

        for(let i = 0; i < value.temperature.length; i++)
        {
            const newDate = addHoursToDate(baseDate, i);

            if(!newDate.getHours())
                result += '\n';

            if(newDate < addHoursToDate(new Date(),  -1))
                continue;

            const time = `${newDate.toLocaleTimeString('en-US', timeStringOptions)}`.substring(0, 5);
            const temperature = `🌡️${parseInt(weighted(value.temperature[i])).toString().padStart(3)}ºF`;
            const dewpoint = `💧${parseInt(weighted(value.dewpoint[i])).toString().padStart(3)}ºF`;
            const hourTotal = weighted(value.precip[i], 'hourTotal');
            const rate = weighted(value.precip[i], 'rate');
            const pressure = `${weighted(value.pressure[i]).toFixed(2)}`;
            const lightning = parseInt(Math.floor(weighted(value.lightning[i])));
            const itcc = parseInt(weighted(value.totalCloudCover[i]));
            const tcc = `${tccEmoji(itcc, lightning, value.precip[i][index].types, rate)}`;
            const visibility = `${parseInt(weighted(value.vis[i]))}`.padStart(2);
            const windDirection = dir(parseInt(weighted(value.wind[i], 'dir')), false);
            const windSpeed = `${parseInt(weighted(value.wind[i], 'speed'))}`.padStart(2);
            const windGust = `${parseInt(weighted(value.wind[i], 'gust'))}`.padStart(2);;

            result += `${time}｜${visEmoji(visibility, value.coords, newDate)}｜${temperature} ${dewpoint}｜${tcc} ${hourTotal.toFixed(3)}｜${windDirection} @ ${windSpeed} G ${windGust}｜${pressure}"\n`;
        }
        return result;
    }).join('\n');
}

const renderForHour = (hour) => {
    const hrrr = JSON.parse(fs.readFileSync(`./forecasts/hrrr-${hour}.json`, 'utf8'));
    const baseDate = new Date(hrrr.date);

    console.log(`${prettyDate(baseDate)} ${prettyTime(baseDate)} Forecast
${tableBody(baseDate, hrrr)}`);
}

if(process.argv.length === 3)
{
    const hour = parseInt(process.argv[2]);
    if(isNaN(hour) || hour < 0 || hour > 23) {
        console.log("Hour must be a value from 0 to 23s")
        return
    }
    renderForHour(hour);
}
else {
    const hour = fs.readFileSync('./forecasts/lastRun', 'utf8');
    renderForHour(hour);
}