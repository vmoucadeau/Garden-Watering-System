var dayschecked = [0,0,0,0,0,0,0];
var vlistdisplayed = false;

// ----------------------------------------------------------------- TOOLS FUNCTIONS

// Used to add minutes to a Date object.
var add_minutes =  function (dt, minutes) {
    return new Date(dt.getTime() + minutes*60000);
}

// Warn if overriding existing method
if(Array.prototype.equals)
    console.warn("Overriding existing Array.prototype.equals. Possible causes: New API defines the method, there's a framework conflict or you've got double inclusions in your code.");
// attach the .equals method to Array's prototype to call it on any array
Array.prototype.equals = function (array) {
    // if the other array is a falsy value, return
    if (!array)
        return false;

    // compare lengths - can save a lot of time 
    if (this.length != array.length)
        return false;

    for (var i = 0, l=this.length; i < l; i++) {
        // Check if we have nested arrays
        if (this[i] instanceof Array && array[i] instanceof Array) {
            // recurse into the nested arrays
            if (!this[i].equals(array[i]))
                return false;       
        }           
        else if (this[i] != array[i]) { 
            // Warning - two different object instances will never be equal: {x:20} != {x:20}
            return false;   
        }           
    }       
    return true;
}
// Hide method from for-in loops
Object.defineProperty(Array.prototype, "equals", {enumerable: false});

var getJSON = function(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.responseType = 'json';
    xhr.onload = function() {
      var status = xhr.status;
      if (status === 200) {
        callback(null, xhr.response);
      } else {
        callback(status, xhr.response);
      }
    };
    xhr.send();
};

function getDate() {
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function() {
        if(this.readyState == 4 && this.status == 200) {
            var json = this.responseText;
            var obj = JSON.parse(json);
            var date = obj.day + "/" + obj.month + "/" + obj.year;
            var hour = obj.hour + "h" + obj.minute + "m" + obj.sec + "s";
            var temp = obj.temperature;
            document.getElementById("date").innerHTML = "Date : " + date + " | " + hour + "<br> Température : " + temp + "°C";
        }
    };

    xhttp.open("GET", "datetime.json", true);
    xhttp.send();

}

function setDate() {
    var timeprovider = $("#timeprovider").val();
    if(timeprovider == "rtc") {
        var date = new Date($('#datetoset').val());
        var time = $('#hourtoset').val().split(':');
        var timezone = $('#timezonertc').val();
        
        date.setHours(parseInt(time[0], 10) + parseInt(timezone, 10), parseInt(time[1], 10))
        
        
        $.post("SetRTCTime", {
            timestamp: date.getTime()/1000
        })
        .done(function() {
            setTimeout(getDate(), 1000);
            alert("Opération effectuée")
            $("#ModalSetTime").modal("hide");
        })
        .fail(function() {
            alert("Il y a eu un problème lors de l'envoi des données.")
        });
    }
    else if(timeprovider == "ntp") {
        var ntpurl = $('#ntpurl').val();
        var timezone = $('#timezonentp').val();
        var smt = 0;
        if($("#summertime").is(":checked")) {
            smt = 1;
        }
        $.post("SetNTPTime", {
            ntpserver: ntpurl,
            timezone: timezone,
            summertime: smt
        })
        .done(function() {
            setTimeout(getDate(), 1000);
            alert("Opération effectuée")
            $("#ModalSetTime").modal("hide");
        })
        .fail(function() {
            alert("Il y a eu un problème lors de l'envoi des données.")
        });
    }
}

$("#timesave").click(function() {
    setDate();
});

$("#timeprovider").change(function() {
    if($("#timeprovider").val() == "ntp") {
        $("#showrtc").hide();
        $("#showntp").show();
        
    }
    if($("#timeprovider").val() == "rtc") {
        $("#showrtc").show();
        $("#showntp").hide();
    }
});

function WiFiSave() {
    var mode = $("#wifimode").val();
    var ssid = $('#wifissid').val();
    var password = $('#wifipassword').val();  

    if(mode == null) {alert("Veuillez choisir un mode WiFi."); return;}
    if(ssid == "") {alert("Le SSID du WiFi ne peut pas être vide."); return;}
    if(password == "") {alert("Le mot de passe du WiFi ne peut pas être vide."); return;}

    $.post("SetWiFiMode", {
        mode: mode,
        ssid: ssid,
        password: password
    })
    .done(function() {
        alert("Opération effectuée");
    })
    .fail(function() {
        alert("Il y a eu un problème lors de l'envoi des données.")
    });
    
    
}

$("#wifisave").click(function() {
    WiFiSave();
});

function setDay() {
    var days = ["monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"];
    dayschecked = [0,0,0,0,0,0,0];

    for(var i = 0; i < days.length; i++) {
        if($("#" + days[i]).is(":checked")) {
            dayschecked[i] = 1;
        }
    }
}

$(':checkbox').change(function() {
    setDay();
});

// ----------------------------------------------------------------- VALVES FUNCTIONS

function InitValves() {
    var xhttp = new XMLHttpRequest();
    
    document.getElementById("valves-list").innerHTML = "";
    document.getElementById("inputev").innerHTML = '<option selected disabled value="RIEN">Choisir une vanne...</option>';
    
    xhttp.onreadystatechange = function() {
        if(this.readyState == 4 && this.status == 200) {
            var json = JSON.parse(this.responseText);
            var type_names = ["Locale", "Distante", "Locale (latching)"];
            for(var id_ev = 0; id_ev < json.length; id_ev++) {
                var obj = json[id_ev];
                var type = type_names[obj.type];
                var valveslisthtml = "<a href='#' class='list-group-item list-group-item-action flex-column align-items-start'> <div class='d-flex w-100 justify-content-between'> <h5 class='mb-1'>" + obj.name + "</h5> <small>" + type + " | " + id_ev + " </small> </div> <p class='statelabel'>Etat : </p>"
                
                 
                if (!obj.state) { 
                    valveslisthtml += "<p class='stateoff'>OFF</p>" 
                } 
                else { 
                    valveslisthtml += "<p class='stateon'>ON</p>"; 
                }
                valveslisthtml += "<br> <p>Démarrage manuel : <br><input type='number' id='manustart" + id_ev + "' value='2' min='2' max='300'> minutes <button type='button' onclick='TemporaryCycle(" + id_ev + ");' class='btn btn-outline-primary btn-sm'>Valider</button></p> <br> <button style='float: left;' type='button' onclick='StartValve(" + id_ev + ");' class='btn btn-primary btn-sm'>Ouvrir la vanne</button>  <button style='float: left; margin-left: 10px;' type='button' onclick='StopValve(" + id_ev + ");' class='btn btn-danger btn-sm'>Fermer la vanne</button>  <br> <button style='float: right;' type='button' onclick='DeleteValve(" + id_ev + ");' class='btn btn-outline-danger btn-sm'>Supprimer</button> </a> ";
                
                document.getElementById("valves-list").innerHTML += valveslisthtml;

                document.getElementById("inputev").innerHTML += '<option value="' + id_ev + '">' + obj.name + '</option>';
            }
            
            
        }
    };
    xhttp.open("GET", "valves.json", true);
    xhttp.send();
}

function StartValve(id_ev) {
    getJSON('valves.json', function(err, data) {
        if(err !== null) {
            alert("Il y a un problème : " + err);
        }
        else {
            if (window.confirm('Voulez-vous vraiment ouvrir la vanne "' + data[id_ev].name + '" ?'))
            {
                $.post("StartValve", {
                    id: id_ev
                })
                .done(function() {
                    setTimeout(InitCycles(), 1000);
                    setTimeout(InitValves(), 1000);
                })
                .fail(function() {
                    alert("Il y a eu un problème lors de l'ouverture de la vanne.")
                });
                
            }  
        }
        
    });
}

function StopValve(id_ev) {
    getJSON('valves.json', function(err, data) {
        if(err !== null) {
            alert("Il y a un problème : " + err);
        }
        else {
            if (window.confirm('Voulez-vous vraiment fermer la vanne "' + data[id_ev].name + '" ?'))
            {
                $.post("StopValve", {
                    id: id_ev
                })
                .done(function() {
                    setTimeout(InitCycles(), 1000);
                    setTimeout(InitValves(), 1000);
                })
                .fail(function() {
                    alert("Il y a eu un problème lors de la fermeture de la vanne.")
                });
                
            } 
        }
        
    });
}

function DeleteValve(id_ev) {
    getJSON('valves.json', function(err, data) {
        if(err !== null) {
            alert("Il y a un problème : " + err);
        }
        else {
            if (window.confirm('Voulez-vous vraiment supprimer la vanne "' + data[id_ev].name + '" ?'))
            {
                $.post("DeleteValve", {
                    id: id_ev
                })
                .done(function() {
                    setTimeout(InitCycles(), 1000);
                    setTimeout(InitValves(), 1000);
                })
                .fail(function() {
                    alert("Il y a eu un problème lors de la suppression de la vanne.")
                });
                
            }
        }
        
    });
}

function AddValve(name, type, pin1, pin2, starturl, stopurl) {
    $.post('AddValve', {
        name: name || "Valve",
        type: type || 0,
        pin1: pin1 || 0,
        pin2: pin2 || 0,
        starturl: starturl || "",
        stopurl: stopurl || ""
    })
    .done(function() {
        setTimeout(InitValves(), 1000);
    })
    .fail(function() {
        alert("Il y a eu un problème lors de l'envoi des informations de la nouvelle vanne.")
    });
    
} 

$("#vannetype").change(function() {
    if($("#vannetype").val() == "locallatching") {
        $("#showlocal").hide();
        $("#showdistante").hide();
        $("#showlocallatching").show();
        
    }
    if($("#vannetype").val() == "local") {
        $("#showlocallatching").hide();
        $("#showdistante").hide();
        $("#showlocal").show();
    }
    if($("#vannetype").val() == "distante") {
        $("#showlocallatching").hide();
        $("#showlocal").hide();
        $("#showdistante").show();
    }
});

$( "#vannesave" ).click(function() {
    var name = $("#valvename").val();
    if(name == "") {alert("Veuillez définir un nom pour la vanne"); return;}
    var type = $("#vannetype option:selected").val();
    if(type == "RIEN") {alert("Veuillez choisir un type d'électrovanne"); return;}
    
    if(type == "local") {
        var startpin = $("#startpin").val();
        AddValve(name, 0, startpin);
    }
    if(type == "locallatching") {
        var Hpin1 = $("#Hpin1").val();
        var Hpin2 = $("#Hpin2").val();
        AddValve(name, 2, Hpin1, Hpin2);
    }
    if(type == "distante") {
        var starturl = $("#starturl").val();
        var stopurl = $("#stopurl").val();
        if(starturl == "") {alert("Vous devez renseignez une URL de démarrage."); return;}
        if(stopurl == "") {alert("Vous devez renseignez une URL d'arrêt."); return;}
        AddValve(name, 1, 0, 0, 0, starturl, stopurl);
    }    

    $('#ModalCreateEV').modal('hide');
});




// ----------------------------------------------------------------- CYCLES FUNCTIONS

function InitCycles() {
    
    document.getElementById("cycles").innerHTML = "";
    
    getJSON('valves.json', function(err, datavalves) {
        if (err !== null) {
            alert('Something went wrong: ' + err);
        } else {
            getJSON('schedules.json',
                function(err, dataschedules) {
                    if (err !== null) {
                        alert('Something went wrong: ' + err);
                    } else {  
                        for(var id_ev = 0; id_ev < datavalves.length; id_ev++) {
                            var objvalve = datavalves[id_ev];
                            var cyclelistinner = "";
                            var hascycle = false;
                            cyclelistinner += "<div class=\"settingstitle\"> <h2 style=\"display: inline;\" class=\"tab_title\">" + objvalve.name + "</h2> </div>";
                            for(var id_prog = 0; id_prog < dataschedules.length; id_prog++) {
                                var objschedule = dataschedules[id_prog];
                                if(objschedule.id_ev == id_ev) {
                                    hascycle = true;
                                    var objdays = objschedule.daysActive;
                                    var daysarray = []
                                    var daysen = ["monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"];
                                    var daysfr = ["Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche"];
                                    for(day in daysen) {
                                        if(objdays[daysen[day]]) {
                                            daysarray.push(daysfr[day]);
                                        }
                                    }

                                    cyclelistinner += '<div class="list-group" style="margin-top: 10px;"> <a href="#" class="list-group-item list-group-item-action flex-column align-items-start"> <div class="d-flex w-100 justify-content-between"> <h5 class="mb-1"> ' + objschedule.name + ' </h5> <button type="button" onclick="DeleteCycle(' + id_prog + ');" class="btn btn-outline-danger btn-sm">Supprimer</button> </div> <p class="hstart">Heure début : ' + objschedule.Hourstart + 'h' + objschedule.Minstart + '  </p> <p class="hstop">Heure fin : ' + objschedule.Hourstop + 'h' + objschedule.Minstop + ' </p> ' ;
                                    if(!objschedule.temporary) {
                                        cyclelistinner += '<p>Jour(s) :  ' + daysarray.join(', ') + ' </p> </a> </div>';
                                    }
                                    
                                } 
                                
                            }
                            if(hascycle) {
                                document.getElementById("cycles").innerHTML += cyclelistinner;
                            }              
                        }                     
                        
                    }
                }
            );
        }
    });
}

function DeleteCycle(id_prog) {
    getJSON('schedules.json', function(err, data) {
        if(err !== null) {
            alert("Il y a un problème : " + err);
        }
        else {
            if (window.confirm('Voulez-vous vraiment supprimer le cycle "' + data[id_prog].name + '" ?'))
            {
                $.post("DeleteCycle", {
                    id: id_prog
                })
                .done(function() {
                    setTimeout(InitCycles(), 1000);
                })
                .fail(function() {
                    alert("Il y a eu un problème lors de la suppression du cycle.")
                });
                
            } 
        }
        
    });
}

function AddCycle(name, id_valve, StartHour, EndHour, days, temp) {
    $.post('AddCycle', {
        name: name,
        id_ev: id_valve,
        starth: StartHour.getHours(),
        startm: StartHour.getMinutes(),
        endh: EndHour.getHours(),
        endm: EndHour.getMinutes(),
        monday: days[0],
        tuesday: days[1],
        wednesday: days[2],
        thursday: days[3],
        friday: days[4],
        saturday: days[5],
        sunday: days[6],
        temporary: temp
    })
    .done(function() {
        setTimeout(InitCycles(), 1000);
    })
    .fail(function() {
        alert("Il y a eu un problème lors de l'envoi des informations du nouveau cycle.")
    });
} 

$( "#cyclesave" ).click(function() {
    var name = $("#cyclename").val();
    if(name == "") {alert("Veuillez définir un nom pour le cycle"); return;}
    var id_ev = $("#inputev option:selected").val();
    if(id_ev == "RIEN") {alert("Veuillez choisir une électrovanne pour le cycle"); return;}
    var start = $("#starthour").val();
    if(start == "") {alert("Veuillez choisir une heure de début pour le cycle"); return;}
    var hstart = parseInt(start.split(':')[0], 10);
    var mstart = parseInt(start.split(':')[1], 10);
    var end = $("#endhour").val();
    if(end == "") {alert("Veuillez choisir une heure de fin pour le cycle"); return;}
    var hend = parseInt(end.split(':')[0], 10);
    var mend = parseInt(end.split(':')[1], 10);

    var StartHour = new Date();
    StartHour.setHours(hstart, mstart, 0);
    var EndHour = new Date();
    EndHour.setHours(hend, mend, 0);
    if(EndHour < StartHour) {alert("L'heure de fin doit être supérieure à l'heure de début"); return;}

    var falsearray = [0, 0, 0, 0, 0, 0, 0];
    if(dayschecked.equals(falsearray)) {alert("Veuillez sélectionner au moins un jour"); return;}

    
    if (window.confirm('Voulez-vous créer le cycle "' + name + '" ? \nValeurs : \nNom : ' + name + "\n" + "Vanne : " + id_ev + "\n" + "Heure début : " + StartHour.getHours() + "h" + StartHour.getMinutes() + "\n" + "Heure de fin : " + EndHour.getHours() + "h" + EndHour.getMinutes() + "\n" + "Jours : " + dayschecked)) {
        $('#ModalCreateSchedule').modal('hide');
        AddCycle(name, id_ev, StartHour, EndHour, dayschecked, 0);
    }    

    $('#ModalCreateSchedule').modal('hide');
});

function TemporaryCycle(id_ev) {

    var minutes = $("#manustart" + id_ev).val();
    var datestart = new Date();
    var datestop = add_minutes(datestart, minutes);
    var daystemp = [1, 1, 1, 1, 1, 1, 1];
    
    getJSON('valves.json', function(err, data) {
        if(err !== null) {
            alert("Il y a un problème : " + err);
        }
        else {
            if(window.confirm("Voulez-vous lancer un cycle temporaire de " + minutes + " minutes pour la vanne \"" + data[id_ev].name + "\" ?")) {
                AddCycle("Cycle temporaire", id_ev, datestart, datestop, daystemp, true);
            }
        }
        
    });
}




// ----------------------------------------------------------------- OTHERS FUNCTIONS


$(document).ready(function() {
    getDate();
    InitValves();
    InitCycles();
    $("#showsettime").click(function() {
        var date = new Date();
        var currentDate = date.toISOString().slice(0,10);
        var currentTime = date.getHours() + ':' + date.getMinutes();
    
        document.getElementById('datetoset').value = currentDate;
        document.getElementById('hourtoset').value = currentTime;
    })
});

setInterval(getDate, 3000);