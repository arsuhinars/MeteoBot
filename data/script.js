
let mainContainer
let weatherContainer

let spinner
let weatherText

let temperatureText
let humidityText
let pressureText
let forecastText

let chart

let temperatureDataset = {
    label: 'Температура (°C)',
    borderColor: '#E42727',
    backgroundColor: '#E42727',
    tension: 0.3,
}
let pressureDataset = {
    label: 'Давление (гПа)',
    borderColor: '#B1E427',
    backgroundColor: '#B1E427',
    tension: 0.3,
}
let humidityDataset = {
    label: 'Влажность (%)',
    borderColor: '#27D7E4',
    backgroundColor: '#27D7E4',
    tension: 0.3,
}
let forecastDataset = {
    label: 'Прогноз (попугаи)',
    borderColor: '#FFB037',
    backgroundColor: '#FFB037',
    tension: 0.3,
}

const month = ['Январь', 'Февраль', 'Март', 'Апрель', 'Май', 'Июнь',
    'Июль', 'Август', 'Сентябрь', 'Октябрь', 'Ноябрь', 'Декабрь']
const forecast = [
    "Отличная, ясно",
    "Хорошая, ясно",
    "Хорошая, но ухудшается",
    "Достаточно хорошая, но ожидается ливень",
    "Ливни, становится менее устойчивой",
    "Пасмурная, ожидаются дожди",
    "Временами дожди, ухудшение",
    "Временами дожди, очень плохая, пасмурно",
    "Дожди, очень плохая, пасмурно",
    "Штормовая, дожди"
]

function decodeWeatherData(buffer) {
    let output = []
    let length = Math.floor(buffer.byteLength / 20)
    let dataView = new DataView(buffer)

    for (let i = 0; i < length; i++) {
        output.push({
            temp: dataView.getFloat32(i * 20, true),
            pressure: dataView.getFloat32(i * 20 + 4, true),
            humidity: dataView.getFloat32(i * 20 + 8, true),
            forecast: dataView.getFloat32(i * 20 + 12, true),
            measureTime: dataView.getUint32(i * 20 + 16, true)
        })
    }

    return output
}

function showWeatherPeriod(index) {
    if (index == 0) {
        weatherContainer.classList.remove('d-none')
        chart.canvas.classList.add('d-none')
    } else {
        weatherContainer.classList.add('d-none')
        chart.canvas.classList.remove('d-none')
    }

    switch (index) {
        case 0: {
            spinner.style.display = ''
            weatherText.style.filter = 'blur(5px)'

            fetch('/getCurrentWeather')
                .then((response) => response.arrayBuffer())
                .then((result) => {
                    let weatherData = decodeWeatherData(result)
                    let weather = weatherData[0]

                    spinner.style.display = 'none'
                    weatherText.style.filter = ''

                    let N = Math.min(Math.round(weather['forecast']), 9)

                    temperatureText.innerHTML = weather['temp'].toFixed(1) + ' °C'
                    humidityText.innerHTML = weather['humidity'].toFixed(1) + '%'
                    pressureText.innerHTML = (weather['pressure'] * 0.01).toFixed(1) + ' гПа'
                    forecastText.innerHTML = forecast[N] + ' (' + weather['forecast'].toFixed(1) + ' П)'
                })
            break
        }
        case 1: {
            fetch('/getHourWeather')
            .then((response) => response.arrayBuffer())
            .then((result) => {
                let weatherData = decodeWeatherData(result)
                let temperatureData = []
                let pressureData = []
                let humidityData = []
                let forecastData = []
                
                chart.data.labels = []
                for (let i = 0; i < weatherData.length; i++) {
                    let date = new Date(weatherData[i].measureTime * 1000)
                    if (weatherData[i].measureTime > 0) {
                        chart.data.labels.push(
                            date.getHours().toString().padStart(2, '0') + 
                            ':' +
                            date.getMinutes().toString().padStart(2, '0')
                        )
                    } else {
                        chart.data.labels.push('-')
                    }
                    temperatureData.push(weatherData[i].temp)
                    pressureData.push(weatherData[i].pressure * 0.01)
                    humidityData.push(weatherData[i].humidity)
                    forecastData.push(weatherData[i].forecast)
                }
                
                chart.data.datasets = [
                    Object.assign(temperatureDataset, {
                        data: temperatureData
                    }),
                    Object.assign(pressureDataset, {
                        data: pressureData
                    }),
                    Object.assign(humidityDataset, {
                        data: humidityData
                    }),
                    Object.assign(forecastDataset, {
                        data: forecastData
                    })
                ]
                chart.update()
            })
            break
        }
        case 2: {
            fetch('/getDayWeather')
            .then((response) => response.arrayBuffer())
            .then((result) => {
                let weatherData = decodeWeatherData(result)
                let temperatureData = []
                let pressureData = []
                let humidityData = []
                let forecastData = []
                
                chart.data.labels = []
                for (let i = 0; i < weatherData.length; i++) {
                    let date = new Date(weatherData[i].measureTime * 1000)
                    if (weatherData[i].measureTime > 0) {
                        chart.data.labels.push(
                            date.getHours().toString().padStart(2, '0') + 
                            ':' +
                            date.getMinutes().toString().padStart(2, '0')
                        )
                    }
                    else {
                        chart.data.labels.push('-')
                    }
                    temperatureData.push(weatherData[i].temp)
                    pressureData.push(weatherData[i].pressure * 0.01)
                    humidityData.push(weatherData[i].humidity)
                    forecastData.push(weatherData[i].forecast)
                }
                
                chart.data.datasets = [
                    Object.assign(temperatureDataset, {
                        data: temperatureData
                    }),
                    Object.assign(pressureDataset, {
                        data: pressureData
                    }),
                    Object.assign(humidityDataset, {
                        data: humidityData
                    }),
                    Object.assign(forecastDataset, {
                        data: forecastData
                    })
                ]
                chart.update()
            })
            break
        }
        case 3: {
            fetch('/getMonthWeather')
            .then((response) => response.arrayBuffer())
            .then((result) => {
                let weatherData = decodeWeatherData(result)
                let temperatureData = []
                let pressureData = []
                let humidityData = []
                let forecastData = []
                
                chart.data.labels = []
                for (let i = 0; i < weatherData.length; i++) {
                    let date = new Date(weatherData[i].measureTime * 1000)
                    if (weatherData[i].measureTime > 0) {
                        chart.data.labels.push(
                            date.getDate().toString() + 
                            '.' +
                            (date.getMonth() + 1).toString().padStart(2, '0')
                        )
                    } else {
                        chart.data.labels.push('-')
                    }
                    temperatureData.push(weatherData[i].temp)
                    pressureData.push(weatherData[i].pressure * 0.01)
                    humidityData.push(weatherData[i].humidity)
                    forecastData.push(weatherData[i].forecast)
                }
                
                chart.data.datasets = [
                    Object.assign(temperatureDataset, {
                        data: temperatureData
                    }),
                    Object.assign(pressureDataset, {
                        data: pressureData
                    }),
                    Object.assign(humidityDataset, {
                        data: humidityData
                    }),
                    Object.assign(forecastDataset, {
                        data: forecastData
                    })
                ]
                chart.update()
            })
            break
        }
        case 4: {
            fetch('/getYearWeather')
            .then((response) => response.arrayBuffer())
            .then((result) => {
                let weatherData = decodeWeatherData(result)
                let temperatureData = []
                let pressureData = []
                let humidityData = []
                let forecastData = []
                
                chart.data.labels = []
                for (let i = 0; i < weatherData.length; i++) {
                    let date = new Date(weatherData[i].measureTime * 1000)
                    if (weatherData[i].measureTime > 0) {
                        chart.data.labels.push(month[date.getMonth()])
                    } else {
                        chart.data.labels.push('-')
                    }
                    temperatureData.push(weatherData[i].temp)
                    pressureData.push(weatherData[i].pressure * 0.01)
                    humidityData.push(weatherData[i].humidity)
                    forecastData.push(weatherData[i].forecast)
                }
                
                chart.data.datasets = [
                    Object.assign(temperatureDataset, {
                        data: temperatureData
                    }),
                    Object.assign(pressureDataset, {
                        data: pressureData
                    }),
                    Object.assign(humidityDataset, {
                        data: humidityData
                    }),
                    Object.assign(forecastDataset, {
                        data: forecastData
                    })
                ]
                chart.update()
            })
            break
        }
    }
}

document.addEventListener('DOMContentLoaded', () => {
    mainContainer = document.querySelector('main')
    weatherContainer = document.getElementById('weather-container')

    spinner = document.getElementById('spinner')
    weatherText = document.getElementById('weather-text')

    temperatureText = document.getElementById('temperature-text')
    humidityText = document.getElementById('humidity-text')
    pressureText = document.getElementById('pressure-text')
    forecastText = document.getElementById('forecast-text')

    chart = new Chart(
        document.getElementById('chart-canvas'), {
        type: 'line',
        options: {
            responsive: true,
            aspectRatio: 1.5,
            scales: {
                y: {
                    suggestedMin: 0,
                    suggestedMax: 100
                }
            }
        }
    })

    let weatherPeriod = document.getElementById('weather-period')
    weatherPeriod.selectedIndex = 0
    weatherPeriod.onchange = (e) =>
        showWeatherPeriod(e.target.selectedIndex)
    showWeatherPeriod(0)

    fetch('/getInfo')
        .then((response) => response.json())
        .then((result) => {
            let telegramLink = document.getElementById('telegram-link')
            telegramLink.setAttribute('href', 'https://t.me/' + result.botName)
            telegramLink.innerHTML = '@' + result.botName

            let geoLocation = document.getElementById('geo-location')
            geoLocation.innerHTML = 'Погода в ' + result.geoLocation
        })
})
