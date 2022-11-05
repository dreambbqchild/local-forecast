const fs = require('fs');
const { getSunrise, getSunset } = require('sunrise-sunset-js');
const timeZone = 'America/Chicago';
const timeStringOptions = { timeZone: timeZone, hour: 'numeric' };

const config = {
    hour: fs.readFileSync('./forecasts/lastRun', 'utf8'),
    splitIntoFiles: false,
    maxForecastLength: 48
};

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
        if(precipTypes.length > 1 || precipTypes[0] === 'freezing rain')
            return '🧊';
        else if(precipTypes[0] === 'rain')
            return '🌧️';
        else if(precipTypes[0] === 'snow')
            return '🌨️';

        return '';
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

    if(sunrise.getHours() == date.getHours() || sunset.getHours() == date.getHours())
        return '🌆';

    if(sunrise >= date || sunset <= date)
        return '🌃';
    
    return '🏙️';
}

const tableBody = (baseDate, hrrr) => 
{
    let forecastIndex = 0;
    const now = new Date();
    return Object.entries(hrrr.locations).sort((l, r) =>  l[1].coords.lon - r[1].coords.lon).map(arr =>
    {
        const [key, value] = arr;
        let result = `${key} 🏠\n`;
        const index = findNearest(value);
        const weights = findWeightsForAverage(value);
        const weightsSum = weights.reduce((a, b) => a + b, 0);
        let lines = 0;
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

            if(newDate < addHoursToDate(now,  -1))
                continue;

            if(!newDate.getHours())
                result += '\n';            

            const time = `${newDate.toLocaleTimeString('en-US', timeStringOptions)}`.replace(/ ([A|P])M/, '$1').padStart(3);
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

            if(++lines === config.maxForecastLength)
                break;
        }
    
        if(config.splitIntoFiles) {
            if(!fs.existsSync('./renderings'))
                fs.mkdirSync('./renderings');

            fs.writeFileSync(`./renderings/${forecastIndex.toString().padStart(2, '0')}.txt`, result, 'utf8');
        }

        forecastIndex++;
        return result;
    }).join('\n');
}

const render = () => {
    const hrrr = JSON.parse(fs.readFileSync(`./forecasts/hrrr-${config.hour}.json`, 'utf8'));
    const baseDate = new Date(hrrr.date);

    console.log(`${prettyDate(baseDate)} ${prettyTime(baseDate)} Forecast
${tableBody(baseDate, hrrr)}`);
}

const testNumber = (str, label, min, max) => {
    const num = parseInt(str);
    if(isNaN(num) || num < min || num > max) {
        console.log(`${label} must be a value from ${min} to ${max}. Got ${str}`);
        return null;
    }

    return num;
}

for(let i = 0; i < process.argv.length; i++)
{    
    if(process.argv[i] === '-s')
        config.splitIntoFiles = true;
    else if(process.argv[i] === '-h'){
        i++;
        config.hour = testNumber(process.argv[i], 'Hour', 0, 23);
        if(config.hour === null)
            return;
    }
    else if(process.argv[i] === '-l')
    {
        i++;
        config.maxForecastLength = testNumber(process.argv[i], 'MaxForecastLength', 1, 48);
        if(config.maxForecastLength === null)
            return;
    }
}

render();