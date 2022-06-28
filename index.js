const hrrr = require(`./forecasts/hrrr-${process.argv[2]}.json`);
const baseDate = new Date(hrrr.date);
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

console.log(`${baseDate.toLocaleDateString('en-US', { timeZone })} ${baseDate.toLocaleTimeString('en-US', timeStringOptions)}`);
for(const [key, value] of Object.entries(hrrr.locations))
{
    console.log(key);
    const index = findNearest(value);
    for(let i = 0; i < value.temperature.length; i++)
    {
        const newDate = addHoursToDate(baseDate, i);

        if(newDate < addHoursToDate(new Date(),  -1))
             continue;

	if(newDate.getHours() === 0)
             console.log(`${newDate.toLocaleDateString('en-US', { timeZone })}`);

        const time = `${newDate.toLocaleTimeString('en-US', timeStringOptions)}`.padStart(8);
        const temperature = `${parseInt(value.temperature[i][index])}ºF`.padStart(6);
        const dewpoint = `${parseInt(value.dewpoint[i][index])}ºF`.padStart(6);
	    const lightning = `${parseInt(Math.floor(value.lightning[i][index]))}`;
        const rate = `${value.precip[i][index].rate.toFixed(5)}`.padStart(8);
        const types = `${value.precip[i][index].types.join()}`.padStart(16);
        const pressure = `${value.pressure[i][index].toFixed(2)}`;
        const tcc = `${parseInt(value.totalCloudCover[i][index])}%`.padStart(4);
        const visibility = `${parseInt(value.vis[i][index])}`.padStart(3);
        const windDirection = `${parseInt(value.wind[i][index].dir)}`.padStart(3);
        const windSpeed = `${parseInt(value.wind[i][index].speed)}`.padStart(3);
        const windGust = `${parseInt(value.wind[i][index].gust)}`.padStart(3);

        console.log(`${time} ${temperature} ${dewpoint} ${rate} ${types} ${lightning} ${pressure} ${tcc} ${visibility} ${windDirection} ${windSpeed} ${windGust}`);
    }
    console.log();
}
