({
    
    onInit: function (component, event, helper) {
        component.set('v.subscription', null);
        const empApi = component.find('empApi');
        var errorHandler = function (message) {
            console.error('Received error ', JSON.stringify(message));
        }.bind(this);
        empApi.onError(errorHandler);
        helper.subscribe(component, event, helper);
    },
    
    initData : function(component, event, helper) {
        component.set('v.dataInitialized', true);
        console.log('Initialized data...');
    },    
    
    initScript : function(component, event, helper) {
        component.set('v.scriptInitialized', true);
        console.log('Initialized chart.js...');
    },  
    
})