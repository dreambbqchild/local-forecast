import fs from 'fs';
import { getSunrise, getSunset } from 'sunrise-sunset-js';
import moment from 'moment';
const timeZone = 'America/Chicago';
const timeStringOptions = { timeZone: timeZone, hour: 'numeric' };

const config = {
    splitIntoFiles: false,
    maxForecastLength: 48,
    startDate: new Date(),
    wxModel: "hrrr"
};

const dir = (deg, useArrows = true) => {
    const arrows = ["↓", "↙", "←", "↖", "↑", "↗", "→", "↘"];
    const dirs = [" N", "NE", " E", "SE", " S", "SW", " W", "NW"];
    const arr = useArrows ? arrows : dirs;
    return `${arr[Math.trunc(((deg + 22) % 360) / 45)]}`;
}

const addHoursToDate = (date, hours) => new Date(date.getTime() + hours * 60 * 60 * 1000);
const prettyDate = (date) => date.toLocaleDateString('en-US', { timeZone, day: '2-digit', month: '2-digit' });
const prettyTime = (date) => date.toLocaleTimeString('en-US', { timeZone });
const tccEmoji = (itcc, lightning, precipType, precipRate) =>
{   
    if(!precipRate && lightning)
        return '🌩';
    else if(precipRate && lightning)
        return '⛈';
    else if(precipType) {
        if(precipType === 'ice')
            return '🧊';
        else if(precipType === 'rain')
            return '🌧️';
        else if(precipType === 'snow')
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
        return '🌇';

    if(sunrise >= date || sunset <= date)
        return '🌃';
    
    return '🏙️';
}

class ExtremeValue
{
    constructor(value, homeOf = '', metadata = 0)
    {
        this.value = value;
        this.homesOf = homeOf ? new Set([homeOf]) : new Set();
        this.metadata = metadata;
    }
}

const tableBody = (forecast) => 
{
    const extremes = {
        lastForecastTime: null,
        temp: { max: new ExtremeValue(Number.MIN_VALUE), min: new ExtremeValue(Number.MAX_VALUE)},
        wind: { max: new ExtremeValue(Number.MIN_VALUE), min: new ExtremeValue(Number.MAX_VALUE)},
        precip: {max: new ExtremeValue(Number.MIN_VALUE), min: new ExtremeValue(Number.MAX_VALUE)}
    };

    const trySaveSplitPart = (fileName, content) => {
        if(!config.splitIntoFiles) 
            return;

        if(!fs.existsSync('./renderings'))
            fs.mkdirSync('./renderings');

        fs.writeFileSync(`./renderings/${fileName}.txt`, content, 'utf8');
    }

    const logExtreme = (key, value, homeOf, metadata = '') => {
        if(value == extremes[key].max.value) 
            extremes[key].max.homesOf.add(homeOf);
        else if(value > extremes[key].max.value)
            extremes[key].max = new ExtremeValue(value, homeOf, metadata);

        if(value == extremes[key].min.value) 
            extremes[key].min.homesOf.add(homeOf);
        else if(value < extremes[key].min.value)
            extremes[key].min = new ExtremeValue(value, homeOf, metadata);
    }

    const printExtremeResults = () => {
        const who = (names) => {
            let arr = [...names];
            if(arr.length === 1)
                return arr[0];

            const last = arr[arr.length - 1];
            arr = arr.slice(0, -1);
            return `${arr.sort().join(', ')} & ${last}`;
        };

        const wasSnow = extremes.precip.max.metadata >= 0.01;
        const hasPrecip = extremes.precip.max.value >= 0.01;

        const precipLine = hasPrecip ? `${who(extremes.precip.max.homesOf)} can expect the most water precipitation with ${extremes.precip.max.value.toFixed(2)}"${wasSnow ? `, which will fall as ${extremes.precip.max.metadata.toFixed(2)}" of snow`: ''}`
                                     : 'No one is expected to see any meaningful precipitation.'

        let result = `Between now and ${extremes.lastForecastTime.toLocaleString('en-US')}
${who(extremes.temp.max.homesOf)} can expect the highest high of ${extremes.temp.max.value}ºF${extremes.temp.max.value <= 0 ? " (but that's not saying much)": ''}
${who(extremes.temp.min.homesOf)} can expect the lowest low of ${extremes.temp.min.value}ºF${extremes.temp.min.value >= 70 ? " (but that's not saying much)": ''}
${who(extremes.wind.max.homesOf)} can expect highest sustained wind of ${extremes.wind.max.value}mph
${precipLine}`;

        trySaveSplitPart('stats', result);
        return result;
    }
    
    let forecastIndex = 0;
    const now = new Date();
    return Object.entries(forecast.locations).sort((l, r) =>  l[1].coords.x - r[1].coords.x).map(arr =>
    {
        const [homeOf, value] = arr;
        const {wx} = value;
        if(value.isCity)
            return;

        let result = '';
        let lines = 0;
        let lastDay = -1;

        for(let i = 0; i < wx.temperature.length && lines < config.maxForecastLength; i++)
        {
            const forecastTime = new Date(forecast.forecastTimes[i]);

            if(forecastTime < addHoursToDate(now,  -1))
                continue;

            extremes.lastForecastTime = forecastTime;
            if(!lines)
            {
                var textDate = prettyDate(forecastTime);
                var currentLocation = `${homeOf} 🏠`;
                result = `${currentLocation}${textDate.padStart(29 - homeOf.length)}\n`;
            }

            if((!forecastTime.getHours() || lastDay !== forecastTime.getDay()) && lines && lines < config.maxForecastLength)
                result += `${prettyDate(forecastTime).padStart(32)}\n`;

            const newSnow = Math.max(0, i === 0 ? 0 : wx.totalSnow[i] - wx.totalSnow[i - 1]);
            lastDay = forecastTime.getDay();
            const time = `${forecastTime.toLocaleTimeString('en-US', timeStringOptions)}`.replace(/ ([A|P])M/, '$1').padStart(3);
            const temperature = `🌡️${wx.temperature[i].toString().padStart(3)}`;
            const dewpoint = `💧${wx.dewpoint[i].toString().padStart(3)}`;
            const hourTotal =  wx.precipType[i] === "snow" ? newSnow : wx.newPrecip[i];
            const rate = wx.precipRate[i];
            const pressure = `${wx.pressure[i].toFixed(2)}`;
            const lightning = wx.lightning[i];
            const itcc = wx.totalCloudCover[i];
            const tcc = `${tccEmoji(itcc, lightning, wx.precipType[i], rate)}`;
            const visibility = wx.vis[i].toString().padStart(2);
            const windDirection = dir(wx.windDir[i], false);
            const windSpeed = wx.windSpd[i].toString().padStart(2);
            const windGust = wx.gust[i].toString().padStart(2);

            result += `${time}｜${visEmoji(visibility, value.coords, forecastTime)}｜${temperature} ${dewpoint}ºF｜${tcc} ${hourTotal.toFixed(2)}"｜${windDirection} @ ${windSpeed} G ${windGust} mph｜${pressure}"\n`;

            logExtreme('temp', wx.temperature[i], homeOf);
            logExtreme('wind', wx.windSpd[i], homeOf);
            logExtreme('precip', wx.totalPrecip[i], homeOf, wx.totalSnow[i]);

            lines++;
        }
        
        trySaveSplitPart(forecastIndex.toString().padStart(2, '0'), result);

        forecastIndex++;
        return result;
    }).join('\n') + printExtremeResults();
}

const render = () => {
    const forecast = JSON.parse(fs.readFileSync(`./forecasts/${config.wxModel}/forecast.json`, 'utf8'));

    console.log(`${tableBody(forecast)}`);
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
    else if(process.argv[i] === '-l')
    {
        i++;
        config.maxForecastLength = testNumber(process.argv[i], 'Max Forecasts', 1, 208);
        if(config.maxForecastLength === null)
            process.exit(1);
    }
    else if(process.argv[i] == '-h')
    {
        i++;
        var hour = testNumber(process.argv[i], 'First hour', 0, 23);
        if(hour === null)
            process.exit(1);
        config.startDate = moment(config.startDate).set({ hour, minute:0, second:0, millisecond:0 }).toDate();
    }
    else if(process.argv[i] == '-m')
    {
        i++;
        const wxModel = process.argv[i].toLowerCase()
        switch(wxModel)
        {
            case "gfs":
            case "hrrr":
                config.wxModel = wxModel;
                break;
            default:
                process.exit(1);
        }
    }
}

render();
