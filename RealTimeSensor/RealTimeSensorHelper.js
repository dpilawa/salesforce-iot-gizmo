({
    
    subscribe: function (component, event, helper) {
        
        const empApi = component.find('empApi');
        const channel = component.get('v.channel');
        const replayId = -1;
        
        var callback = function (message) {
            console.log('Event Received : ' + JSON.stringify(message));
            helper.onReceiveNotification(component, event, helper, message);
        }.bind(this);
        
        empApi.subscribe(channel, replayId, callback).then(function (newSubscription) {
            console.log('Subscribed to channel ' + channel);
            component.set('v.subscription', newSubscription);
        });
        
    },
    
    
    unsubscribe: function (component, event, helper) {
        
        const empApi = component.find('empApi');
        const channel = component.get('v.subscription').channel;
        
        const callback = function (message) {
            console.log('Unsubscribed from channel ' + message.channel);
        };
        
        empApi.unsubscribe(component.get('v.subscription'), callback);
    },
        
    updateChart : function(component, event, helper, variable_name, value) 
    {
        var chart = component.get(variable_name);
        var maxreadouts = component.get('v.maxReadouts');
        var currentdate = new Date();
        
        chart.data.labels.push(((currentdate.getHours() < 10)?"0":"") + currentdate.getHours() + ':' + ((currentdate.getMinutes() < 10)?"0":"") + currentdate.getMinutes() + ":" + ((currentdate.getSeconds() < 10)?"0":"") + currentdate.getSeconds());
        chart.data.datasets[0].data.push(Number(value));
        if (chart.data.labels.length > maxreadouts) {
            for (var i=0; i<chart.data.labels.length-1; i++)
            {
                chart.data.labels[i]=chart.data.labels[i+1];
                chart.data.datasets[0].data[i]=chart.data.datasets[0].data[i+1];
            }
            chart.data.labels.pop();
            chart.data.datasets[0].data.pop();
        }
        chart.update();		        
    },
    
    newChart : function(component, event, helper, element_name, variable_name, vmin, vmax, refmin, refmax, label)
    {
        var element = component.find(element_name).getElement();
        var maxreadouts = component.get('v.maxReadouts');
        
        var myChart = new Chart(element, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    data: [],                 
                    lineTension: 0.3
                }, 
                {
          			data: [],
					type: 'line',
                    pointRadius: 0,
                    borderColor: 'rgba(0, 0, 220, 0.3)',
                    fill: false,
        		},
                {
          			data: [],
					type: 'line',
                    pointRadius: 0,
                    borderColor: 'rgba(0, 0, 220, 0.3)',
                    fill: false
        		}]
            },
            options: {
                tooltips: {
                    enabled: true,
                },
                layout: {
                    padding: {
                        left: 0,
                        right: 0,
                        top: 20,
                        bottom: 0
                    }
                },
                scales: {
                    yAxes: [{
                        ticks: {
                            beginAtZero:true,
                            suggestedMax:vmax,
                            suggestedMin:vmin,
                        },
                        scaleLabel: {
                            display: true,
                            labelString: label,
                        },
                    }],
                },
                legend: {
                    display: false,
                }
            }
        });    
        
        for (var i=0; i<maxreadouts; i++) {
            myChart.data.datasets[0].data.push('');
            myChart.data.datasets[1].data.push(refmin);
            myChart.data.datasets[2].data.push(refmax);
            myChart.data.labels.push('');
        };
        
        component.set(variable_name, myChart);
    },
    
	initCharts : function(component, event, helper) {
        
        var asset = component.get('v.Asset'); 

        if (asset) {
            helper.newChart(component, event, helper, 'tempChart', 'v.TChartObject', 0, 50, asset.Service_Contract__r.Temperature_Min__c, asset.Service_Contract__r.Temperature_Max__c, 'Temperature (C)');
            helper.newChart(component, event, helper, 'humChart', 'v.HChartObject', 0, 100, asset.Service_Contract__r.Humidity_Min__c, asset.Service_Contract__r.Humidity_Max__c, 'Relative Humidity (%)');
            helper.newChart(component, event, helper, 'lightChart', 'v.LChartObject', 0, 1, null, null, 'Light Intensity');  
        }
        
    },
    
    onReceiveNotification: function (component, event, helper, message) {
        
        var t = message.data.payload.Temperature__c;
        var h = message.data.payload.Humidity__c;
        var l = message.data.payload.Light__c;
        var sensor = message.data.payload.Sensor_Id__c;
        
        var dataInitialized = component.get('v.dataInitialized'); 
        var scriptInitialized = component.get('v.scriptInitialized'); 
        var graphsInitialized = component.get('v.graphsInitialized'); 
        
        console.log('Status : ' + dataInitialized + scriptInitialized + graphsInitialized);
        
        if (sensor == component.get('v.Asset.Sensor_Id__c')) {
            
			if (dataInitialized && scriptInitialized)
            {
                if (!graphsInitialized) 
                {
                    console.log('Initializing charts...');
                    helper.initCharts(component, event, helper);
                    component.set('v.graphsInitialized', true); 
                }
                helper.updateChart(component, event, helper, 'v.TChartObject', t);
                helper.updateChart(component, event, helper, 'v.HChartObject', h);
                helper.updateChart(component, event, helper, 'v.LChartObject', l);
            }
        }
    }
    
})