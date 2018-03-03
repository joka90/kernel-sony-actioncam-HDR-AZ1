// Copyright 2007-2009 Sony Corporation.

startChartCount = 0;
startChartMemCount = 0;
timerInterval = 3000;
log_dir = "";

function updateAC()
{
    var ti = dijit.byId('targetInfoCP');
    ti.setHref("kipct.cgi?name=getTargetInfo&dir=".concat(log_dir));
    var f = dijit.byId('FilesCP');
    f.setHref("kipct.cgi?name=getLogDirFiles&dir=".concat(log_dir));
}


function logdirChanged(val)
{
    log_dir = val
    console.debug("Value changed to ["+val+"] ");
    checkInvalidChart();
    updateAC();
}

function checkInvalidChart()
{
    console.debug("checkInvalidChart\n");
    dojo.xhrGet({
        url: 'kipct.cgi',
	handleAs: "json",
        timeout: 5000, //Time in milliseconds
        handle: checkLogDirFilesCallback,
        content: {
	    name: "checkLogDirFiles",
	    dir: log_dir
	}
    });
}
function getLogDirPressed()
{
    console.debug("getLogDirPressed\n");
    dojo.xhrGet({
        url: 'kipct.cgi',
	handleAs: "json",
        timeout: 5000, //Time in milliseconds
        handle: getLogDirCallback,
        content: {
	    name: "getLogDir",
	}
    });
}
function startChartPressed()
{
    dojo.xhrGet({
        url: 'kipct.cgi',
	handleAs: "json",
        timeout: 5000, //Time in milliseconds
        handle: startChartCallback,
        content: {
	    name: "startChart",
	    scc: startChartCount,
	    dir: log_dir
	}
    });
}
function startChartMemPressed()
{

    dojo.xhrGet({
        url: 'kipct.cgi',
	handleAs: "json",
        timeout: 5000, //Time in milliseconds
        handle: startChartMemCallback,
        content: {
	    name: "startChartMeminfo",
	    scc: startChartMemCount,
	    dir: log_dir
	}
    });
}

globalTimer = null
globalTimerStatus = 0;

function startChartTimer()
{
    if (globalTimerStatus == 0) {
	console.debug("timer stop");
	return;
    }
    var chkBox = dijit.byId('cpuChk');
    var chkBoxCtxt = dijit.byId('ctxtChk');
    var chkBoxPrcs = dijit.byId('prcsChk');
    if (chkBox.checked == true || chkBoxCtxt.checked == true ||
	chkBoxPrcs.checked == true) {
	dojo.xhrGet({
            url: 'kipct.cgi',
            handleAs: "json",
            timeout: 5000, //Time in milliseconds
            handle: startChartCallback,
            content: {
		name: "startChartTimer",
		count: 90,
		dir: log_dir
	    }
	});
    }
    var chkBox = dijit.byId('memChk');
    if (chkBox.checked == true) {
	dojo.xhrGet({
            url: 'kipct.cgi',
            handleAs: "json",
            timeout: 5000, //Time in milliseconds
            handle: startChartMemCallback,
            content: {
		name: "startChartTimerMeminfo",
		count: 90,
		dir: log_dir
	    }
	});
    }

    if (globalTimerStatus == 1) {
	console.debug("timer continue");
	startTimerPressed();
    }

}

function startTimerPressed()
{
    var startbutton = dijit.byId('startTimerButton');
    startbutton.setDisabled(true);
    var stopbutton = dijit.byId('stopTimerButton');
    stopbutton.setDisabled(false);
    globalTimerStatus = 1;
    globalTimer = setTimeout(startChartTimer, timerInterval);
}
function stopTimerPressed()
{
    globalTimerStatus = 0;
    if (globalTimer != null) {
	clearTimeout(globalTimer);
    }
    globalTimer = null;
    var startbutton = dijit.byId('startTimerButton');
    startbutton.setDisabled(false);
    var stopbutton = dijit.byId('stopTimerButton');
    stopbutton.setDisabled(true);
}
function comboListUpdatePressed()
{
    var s = new dojo.data.ItemFileReadStore({url:"kipct.cgi?name=getLogDir"});
    var combo = dijit.byId("logdir");
    combo.store = s;
}

function getLogDirCallback(response, ioArgs)
{
    console.debug("getLogDirCallback\n");
    if(response instanceof Error) {
	if(response.dojoType == "cancel"){
		//The request was canceled by some other JavaScript code.
	    console.debug("Request canceled.");
	}else if(response.dojoType == "timeout"){
		//The request took over 5 seconds to complete.
	    console.debug("Request timed out.");
	}else{
		//Some other error happened.
	    console.error(response);
	}
    } else {
	console.debug("getLogDirCallBack\n");
    }
}

function checkLogDirFilesCallback(response, ioArgs)
{
    console.debug("checkLogDirFilesCallback\n");
    if(response instanceof Error) {
	if(response.dojoType == "cancel"){
		//The request was canceled by some other JavaScript code.
	    console.debug("Request canceled.");
	}else if(response.dojoType == "timeout"){
		//The request took over 5 seconds to complete.
	    console.debug("Request timed out.");
	}else{
		//Some other error happened.
	    console.error(response);
	}
    } else {
	console.debug("checkLogDirFilesCallback\n");
	var invalidList = response.invalid;
	for (var i in invalidList) {
	    var chk = dijit.byId(invalidList[i]);
	    chk.setChecked(false);
	}
	var chkBox = dijit.byId('cpuChk');
	var chkBoxCtxt = dijit.byId('ctxtChk');
	var chkBoxPrcs = dijit.byId('prcsChk');
	if (chkBox.checked == true || chkBoxCtxt.checked == true ||
	    chkBoxPrcs.checked == true) {
		console.debug("cpu or ctxt or prcs Checked\n");
		startChartPressed();
	    }
	var chkBox = dijit.byId('memChk');
	if (chkBox.checked == true) {
	    console.debug("memChecked\n");
	    startChartMemPressed();
	}
    }
}

chartCPU = null;
chartCTXT = null;
chartPRCS = null;

function startChartCpu(response)
{
	if (chartCPU == null) {
	    console.debug("startChartCpu new\n");
	    chartCPU = new Ki.Chart.Cpu("chartCpu", "cpu chart");
	    chartCPU.inputCoordinateFirst(response);
	}
	else {
	    console.debug("startChartCpu inputCoordinate\n");
	    chartCPU.inputCoordinate(response);
	}
}
function startChartCtxt(response)
{
	if (chartCTXT == null) {
	    console.debug("startChartCtxt new\n");
	    chartCTXT = new Ki.Chart.Ctxt("chartCtxt", "ctxt chart");
	    chartCTXT.inputCoordinateFirst(response);
	}
	else {
	    console.debug("startChartCtxt inputCoordinate\n");
	    chartCTXT.inputCoordinate(response);
	}
}
function startChartPrcs(response)
{
	if (chartPRCS == null) {
	    console.debug("startChartPrcs new\n");
	    chartPRCS = new Ki.Chart.Prcs("chartPrcs", "prcs chart");
	    chartPRCS.inputCoordinateFirst(response);
	}
	else {
	    console.debug("startChartPrcs inputCoordinate\n");
	    chartPRCS.inputCoordinate(response);
	}
}

function startChartCallback(response, ioArgs)
{
    console.debug("startChartCallback\n");
    if(response instanceof Error) {
	if(response.dojoType == "cancel"){
		//The request was canceled by some other JavaScript code.
	    console.debug("Request canceled.");
	}else if(response.dojoType == "timeout"){
		//The request took over 5 seconds to complete.
	    console.debug("Request timed out.");
	}else{
		//Some other error happened.
	    console.error(response);
	}
    } else {
	var chkBox = dijit.byId('cpuChk');
	var chkBoxCtxt = dijit.byId('ctxtChk');
	var chkBoxPrcs = dijit.byId('prcsChk');
	if (chkBox.checked == true) {
	    startChartCpu(response);
	}
	if (chkBoxCtxt.checked == true) {
	    startChartCtxt(response);
	}
	if (chkBoxPrcs.checked == true) {
	    startChartPrcs(response);
	}
	console.debug("startChartCallback inputCoordinate\n");
    }
}

chartMEM = null;

function startChartMemCallback(response, ioArgs)
{
    console.debug("startChartMemCallback\n");
    if(response instanceof Error) {
	if(response.dojoType == "cancel"){
		//The request was canceled by some other JavaScript code.
	    console.debug("Request canceled.");
	}else if(response.dojoType == "timeout"){
		//The request took over 5 seconds to complete.
	    console.debug("Request timed out.");
	}else{
		//Some other error happened.
	    console.error(response);
	}
    } else {
	if (chartMEM == null) {
	    console.debug("startChartMemCallback new\n");
	    console.debug("startChartCallback chartMem initialized\n");
	    chartMEM = new Ki.Chart.Mem("chartMem", "cpu chart");
	    chartMEM.inputCoordinateFirst(response);
	}
	else {
	    console.debug("startChartMemCallback inputCoordinate\n");
	    chartMEM.inputCoordinate(response);
	}
	console.debug("startChartMEMCallback inputCoordinate\n");
    }
    console.debug("startChartMEMCallback end\n");
}

function setup_class() {

dojo.declare("Ki.Chart", null, {
    constructor: function(name, desc) {
	console.debug("dojo.declare Ki.Chart:" + "name = " + name + ", desc = " + desc);
	this.name = name;
	this.chart = new dojox.charting.Chart2D(name);
//	this.chart.setTheme(dojox.charting.themes.PlotKit.red);
	this.chart.addPlot("back_grid", {type: "Grid"});
	console.debug("dojo.declare Ki.Chart: end");
    },
    addAxises: function(axis) {
	this.chart.addAxis("x", {fixLower: "none", fixUpper: "none", "min": axis.x.min, "max": axis.x.max});
	this.chart.addAxis("y", {vertical: true, "min": axis.y.min, "max": axis.y.max});
    },
    addSeries: function(name, xydata, clr) {
	this.chart.addSeries(name, xydata, {stroke: {color: clr, width: 1}});
    },
    setLegendFont: function(text) {
	text.setFont({family: "Times", size: "16pt", weight: "bold"});
	text.setStroke("lightgray");
    },
    crSurface: function(name) {
	return dojox.gfx.createSurface(name, 170, 170);
    },
    surCrText: function(sfc, text, color, stc) {
	var t = sfc.createText(text);
	t.setFill(color);
	this.setLegendFont(t);
	if (stc == 1) {
	    var xoffset = text["x"] + t.getTextWidth() + 2;
	    var stc_t = sfc.createText({x: xoffset, y: text["y"], text: "(stacked)", align: text["align"]});
	    stc_t.setFill("black");
	    this.setLegendFont(stc_t);
	}
    },
    createTable: function(table, coordinates, titleId, surId) {
	dojo.byId(titleId).innerHTML = table.title;
	var surface = this.crSurface(surId);
	var yoffset = 20;
	for (var key in coordinates) {
	    var stc_key = key + "_stc";
	    this.surCrText(surface, {x: 10, y: yoffset, text: key, align: "start"}, table.legend[key], table.legend[stc_key]);
	    yoffset += 20;
	}
    },
    updateCoord: function(axis, coordinates) {
	this.addAxises(axis);
	for (var key in coordinates) {
	    this.chart.updateSeries(key, coordinates[key]);
	}
	this.chart.render();
    },
    inputCoord: function(axis, table, coordinates, titleId, surId) {
	this.addAxises(axis);
	for (var key in coordinates) {
	    this.addSeries(key, coordinates[key], table.legend[key]);
	}
	this.chart.render();
	this.createTable(table, coordinates, titleId, surId);
    }
});

dojo.declare('Ki.Chart.Cpu', Ki.Chart, {
    constructor: function(name, desc) {
	console.debug("dojo.declare Ki.Chart.Cpu\n");
    },
    inputCoordinate: function(json) {
	this.updateCoord(json.axis, json.coordinates);
    },
    inputCoordinateFirst: function(json) {
	console.debug("inputCoordinateFirst name=", this.name);
	//console.debug("inputCoordinateFirst json=", dojo.toJson(json, true));
	this.inputCoord(json.axis, json.table, json.coordinates, "cpuTitle", "cpuLegend");
    }
});

dojo.declare('Ki.Chart.Ctxt', Ki.Chart, {
    constructor: function(name, desc) {
	console.debug("dojo.declare Ki.Chart.Ctxt\n");
    },
    inputCoordinate: function(json) {
	this.updateCoord(json.axis_intr_ctxt, json.coordinates_intr_ctxt);
    },
    inputCoordinateFirst: function(json) {
	console.debug("inputCoordinateFirst name=", this.name);
	//console.debug("inputCoordinateFirst json=", dojo.toJson(json, true));
	this.inputCoord(json.axis_intr_ctxt, json.table_intr_ctxt, json.coordinates_intr_ctxt, "ctxtTitle", "ctxtLegend");
    }
});

dojo.declare('Ki.Chart.Prcs', Ki.Chart, {
    constructor: function(name, desc) {
	console.debug("dojo.declare Ki.Chart.Prcs\n");
    },
    inputCoordinate: function(json) {
	this.updateCoord(json.axis_prcs, json.coordinates_prcs);
    },
    inputCoordinateFirst: function(json) {
	console.debug("inputCoordinateFirst name=", this.name);
	//console.debug("inputCoordinateFirst json=", dojo.toJson(json, true));
	this.inputCoord(json.axis_prcs, json.table_prcs, json.coordinates_prcs, "prcsTitle", "prcsLegend");
    }
});


dojo.declare('Ki.Chart.Mem', Ki.Chart, {
    constructor: function(name, desc) {
	console.debug("dojo.declare Ki.Chart.Mem\n");
    },
    inputCoordinate: function(json) {
	this.updateCoord(json.axis, json.coordinates);
    },
    inputCoordinateFirst: function(json) {
	console.debug("inputCoordinateFirst name=", this.name);
	//console.debug("inputCoordinateFirst json=", dojo.toJson(json, true));
	this.inputCoord(json.axis, json.table, json.coordinates, "memTitle", "memLegend");
    }
});

}

function init()
{
    console.debug("init");
    setup_class();
    var startTimer = dijit.byId('startTimerButton');
    var stopTimer = dijit.byId('stopTimerButton');
    var comboListUpdate = dijit.byId('updateComboButton');
    dojo.connect(startTimer, 'onClick', 'startTimerPressed');
    dojo.connect(stopTimer, 'onClick', 'stopTimerPressed');
    dojo.connect(comboListUpdate, 'onClick', 'comboListUpdatePressed');
    console.debug("init done");
}

dojo.addOnLoad(init)

