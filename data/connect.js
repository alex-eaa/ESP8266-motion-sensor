var mqttServer="srv1.mqtt.4api.ru";
var mqttPort=9123;
var mqttUser="user_889afb72";
var mqttPass="pass_7c9ca39a";
var mqttPrefix="user_889afb72";
var mqttDevName="esplink_ms_1adffc";

var mqttState = 0;
var wsState = 0;


if (!location.host){
mqtt = new Paho.MQTT.Client(mqttServer, Number(mqttPort), "");
mqtt.onConnectionLost = onConnectionLost;
mqtt.onMessageArrived = onMessageArrived;

function mqttConnect() {
	mqtt.connect({onSuccess:onConnect,
		onFailure:onFailure, 
		onSuccess:onSuccess, 
		userName:mqttUser, 
		password:mqttPass});
}

function onConnect() {
	console.log("MQTT onConnect");
};

function onSuccess() {
	console.log("MQTT onSuccess");
	mqtt.subscribe(mqttPrefix+"/"+mqttDevName+"/fromDevice");
	mqttState=1;
	startSendData(data["page"]);
};

function onFailure() {
	console.log("MQTT onFailure");
	mqttState=0;
};

function onConnectionLost(responseObject) {
	if (responseObject.errorCode !== 0)
		console.log("MQTT onConnectionLost:"+responseObject.errorMessage);
	mqttState=0;
};

function onMessageArrived(message) {
	console.log("MQTT FROM Server:"+message.payloadString);
	receivedDataProcessing (message.payloadString);
};
}


function wsConnect(wsIP) {
	let wsAdress = 'ws://'+wsIP+':81/'+data['page']+'.htm';
	ws = new WebSocket(wsAdress, ['arduino']);
	
	ws.onopen = function(e) {
  		console.log("WS onConnected");
  		wsState = 1;
  	};

	ws.onclose = function(e) {
		wsState = 0;
		console.log('WS is closed.', e.reason);
	};

	ws.onerror = function (error) {
		wsState = 0;
		console.log('WS ERROR. Reconnect will be attempted in 10 second.');
		setTimeout(function() {
	      wsConnect(wsIP);
	    }, 10000);
	};

	ws.onmessage = function (e) {
		console.log('WS FROM Server: ', e.data);
		receivedDataProcessing (e.data);
	};
}



function startConControl(ip){
	console.log('ip = ', ip);
	changeIndicTypeConnect();

	if (ip!=null || ip!="0")   wsConnect(ip);
	else if(!location.host) mqttConnect();

	setInterval(function() {
		changeIndicTypeConnect();
		if (mqttState==1 && wsState==1 && !location.host){
			mqtt.disconnect();
			mqttState=0;
			console.log('mqtt.disconnect()');
		}
		if (mqttState==0 && wsState==0 && !location.host){
			mqttConnect();
			console.log('mqttConnect()');
		}
	}, 2000);
}



function startSendData(command) {
	switch (command) {
		case "JSON":
			updateDataForSend();
			sendFinishData(JSON.stringify(dataSend));
			break;
		case "index":
			sendFinishData('INDEX');
			break;
		case "setup":
			sendFinishData('SETUP');
			break;
		case "RESET":
			sendFinishData('RESET');
			break;
		case "RESETSTAT":
			sendFinishData('RESETSTAT');
			break;
	}

	function sendFinishData(dat){
		if (wsState==1){
			console.log('WS TO Server: ', dat);
			ws.send(dat);
			unsetIndicConnect();
		}else if (mqttState==1){
			if (mqtt){
				console.log('MQTT TO Server: ', dat);
				let topic = mqttPrefix+"/"+mqttDevName+"/fromClient";
				let qos = 2;
				let retained = false;
				mqtt.send(topic, dat, qos, retained);
			}else{
				console.log('MQTT server not available!');
			}			
			unsetIndicConnect();
		}
	};

}



function receivedDataProcessing(strJson){
	try {
		let obj=JSON.parse(strJson);
		for (x in obj) {
			if (data[x]!=null) {
				data[x]=obj[x];
			} else if (dataSend[x]!=null) {
				dataSend[x]=obj[x];
			}
		}
		updateAllPage();
		setIndicConnect();
	} catch (e) {
		console.log(e.message); 
	}
}


let IPAdress = "192.168.43.159"; 
if (location.host) IPAdress=location.host;
startConControl(IPAdress);
//startConControl();