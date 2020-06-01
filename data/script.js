var monday = 0, tuesday = 0, wednesday = 0, thursday = 0, friday = 0, saturday = 0, sunday = 0;
var vlistdisplayed = false;

// ----------------------------------------------------------------- TOOLS FUNCTIONS

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

setInterval(getDate, 3000);

function setDay() {
    monday = 0, tuesday = 0, wednesday = 0, thursday = 0, friday = 0, saturday = 0, sunday = 0;

    if($("#monday").is(":checked")) {
        monday = 1;
    }
    if($("#tuesday").is(":checked")) {
        tuesday = 1;
    }
    if($("#wednesday").is(":checked")) {
        wednesday = 1;
    }
    if($("#thursday").is(":checked")) {
        thursday = 1;
    }
    if($("#friday").is(":checked")) {
        friday = 1;
    }
    if($("#saturday").is(":checked")) {
        saturday = 1;
    }
    if($("#sunday").is(":checked")) {
        sunday = 1;
    }

}

$(':checkbox').change(function() {
    setDay();
});

// ----------------------------------------------------------------- VALVES FUNCTIONS

function InitValves(reset) {
    var xhttp = new XMLHttpRequest();
    if(reset) {
        document.getElementById("valves-list").innerHTML = "";
        document.getElementById("inputev").innerHTML = '<option selected disabled value="RIEN">Choisir un type...</option>';
    }
    xhttp.onreadystatechange = function() {
        if(this.readyState == 4 && this.status == 200) {
            var json = JSON.parse(this.responseText);
            
            for(var i = 0; i < json.length; i++) {
                var obj = json[i];
                var type;
                if(obj.type == 0) {
                    type = "Locale";
                }
                else if(obj.type == 1) {
                    type = "Distante";
                }
                else if(obj.type == 2) {
                    type = "Locale (latching)";
                }
                else {
                    type = "Inconnu";
                }
                if (obj.state == true) {
                    document.getElementById("valves-list").innerHTML += " <a href='#' class='list-group-item list-group-item-action flex-column align-items-start'> <div class='d-flex w-100 justify-content-between'> <h5 class='mb-1'>" + obj.name + "</h5> <small>" + type + " | " + obj.id_ev + " </small> </div> <p class='statelabel'>Etat : </p><p class='stateon'>ON</p> <br> <small>Démarrage manuel : <input type='number' id='manustart" + obj.id_ev + "' value='1' min='1'> minutes <button type='button' onclick='DeleteValve(" + obj.id_ev + ");' class='btn btn-outline-primary btn-sm'>Valider</button></small><br> <button type='button' onclick='DeleteValve(" + obj.id_ev + ");' class='btn btn-outline-danger btn-sm'>Supprimer</button> </a> ";
                }
                else {
                    document.getElementById("valves-list").innerHTML += " <a href='#' class='list-group-item list-group-item-action flex-column align-items-start'> <div class='d-flex w-100 justify-content-between'> <h5 class='mb-1'>" + obj.name + "</h5> <small>" + type + " | " + obj.id_ev + " </small> </div> <p class='statelabel'>Etat : </p><p class='stateoff'>OFF</p> <br> <small>Démarrage manuel : <input type='number' id='manustart" + obj.id_ev + "' value='1' min='1'> minutes <button type='button' onclick='DeleteValve(" + obj.id_ev + ");' class='btn btn-outline-primary btn-sm'>Valider</button></small> <br> <button type='button' onclick='DeleteValve(" + obj.id_ev + ");' class='btn btn-outline-danger btn-sm'>Supprimer</button> </a> ";
                }
                
                document.getElementById("inputev").innerHTML += '<option value="' + obj.id_ev + '">' + obj.name + '</option>';
            }
            
            
        }
    };
    xhttp.open("GET", "valves.json", true);
    xhttp.send();
}

function DeleteValve(id_ev) {
    getJSON('valves.json', function(err, data) {
        if(err !== null) {
            alert("Il y a un problème : " + err);
        }
        else {
            for(var i = 0; i < data.length; i++) {
                var objdel = data[i];
                if(id_ev == objdel.id_ev) {
                    if (window.confirm('Voulez-vous vraiment supprimer la vanne "' + objdel.name + '" ?'))
                    {
                        $.post("DeleteValve", {
                            id: id_ev
                        })
                        .done(function() {
                            setTimeout(InitCycles(true), 1000);
                            setTimeout(InitValves(true), 1000);
                        })
                        .fail(function() {
                            alert("Il y a eu un problème lors de la suppression de la vanne.")
                        });
                        
                    }    
                }   
            }
        }
        
    });
}

function AddValve(name, type, startpin, Hpin1, Hpin2, starturl, stopurl) {
    if(type == 0) {
        $.post('AddValve', {
            name: name,
            type: type,
            startpin: startpin
        })
        .done(function() {
            setTimeout(InitValves(true), 1000);
        })
        .fail(function() {
            alert("Il y a eu un problème lors de l'envoi des informations de la nouvelle vanne.")
        });
    }
    if(type == 1) {
        $.post('AddValve', {
            name: name,
            type: type,
            starturl: starturl,
            stopurl: stopurl
        })
        .done(function() {
            $('#tab1Id').tab('show');
            setTimeout(InitValves(true), 1000);
        })
        .fail(function() {
            alert("Il y a eu un problème lors de l'envoi des informations de la nouvelle vanne.")
        });
    }
    if(type == 2) {
        $.post('AddValve', {
            name: name,
            type: type,
            Hpin1: Hpin1,
            Hpin2: Hpin2
        })
        .done(function() {
            setTimeout(InitValves(true), 1000);
        })
        .fail(function() {
            alert("Il y a eu un problème lors de l'envoi des informations de la nouvelle vanne.")
        });
    }
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
        if(startpin == 5 || startpin == 4) {alert("Cette pin ne peut pas être utilisée."); return;}
        AddValve(name, 0, startpin);
    }
    if(type == "locallatching") {
        var Hpin1 = $("#Hpin1").val();
        var Hpin2 = $("#Hpin2").val();
        if(Hpin1 == 5 || Hpin1 == 4 ) {alert("La pin " + Hpin1 + " ne peut pas être utilisée."); return;}
        if(Hpin2 == 5 || Hpin2 == 4 ) {alert("La pin " + Hpin2 + " ne peut pas être utilisée."); return;}
        AddValve(name, 2, 0, Hpin1, Hpin2, "", "");
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

function InitCycles(reset) {
    var valvesdisplayed = [];
    if(reset) {
        document.getElementById("cycles").innerHTML = "";
    }
    getJSON('valves.json', function(err, datavalves) {
        if (err !== null) {
            alert('Something went wrong: ' + err);
        } else {
            getJSON('schedules.json',
                function(err, dataschedules) {
                    if (err !== null) {
                        alert('Something went wrong: ' + err);
                    } else {  
                        for(var j = 0; j < datavalves.length; j++) {
                            var objvalve = datavalves[j];
                            for(var i = 0; i < dataschedules.length; i++) {
                                var objschedule = dataschedules[i];
                                
                                if(objschedule.id_ev == objvalve.id_ev) {
                                
                                    if(!valvesdisplayed.includes(objvalve.id_ev)) {
                                        document.getElementById("cycles").innerHTML += "<div class=\"settingstitle\"> <h2 style=\"display: inline;\" class=\"tab_title\">Cycles (" + objvalve.name + ") :</h2> </div>";
                                        valvesdisplayed.push(objvalve.id_ev);
                                    }
                                    
                                    document.getElementById("cycles").innerHTML += '<div class="list-group" style="margin-top: 10px;"> <a href="#" class="list-group-item list-group-item-action flex-column align-items-start"> <div class="d-flex w-100 justify-content-between"> <h5 class="mb-1"> ' + objschedule.name + ' </h5> <button type="button" onclick="DeleteCycle(' + objschedule.id_prog + ');" class="btn btn-outline-danger btn-sm">Supprimer</button> </div> <p class="hstart">Heure début : ' + objschedule.Hourstart + 'h' + objschedule.Minstart + '  </p> <p class="hstop">Heure fin : ' + objschedule.Hourstop + 'h' + objschedule.Minstop + ' </p>  <p>Jour(s) :  </p> </a> </div>';
                                    
                                } 
                                
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
            for(var i = 0; i < data.length; i++) {
                var objdel = data[i];
                if(id_prog == objdel.id_prog) {
                    if (window.confirm('Voulez-vous vraiment supprimer le cycle "' + objdel.name + '" ?'))
                    {
                        $.post("DeleteCycle", {
                            id: id_prog
                        })
                        .done(function() {
                            setTimeout(InitCycles(true), 1000);
                        })
                        .fail(function() {
                            alert("Il y a eu un problème lors de la suppression du cycle.")
                        });
                        
                    }    
                }   
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
        setTimeout(InitCycles(true), 1000);
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

    var daysactive = [monday, tuesday, wednesday, thursday, friday, saturday, sunday];
    var falsearray = [0, 0, 0, 0, 0, 0, 0];
    if(daysactive.equals(falsearray)) {alert("Veuillez sélectionner au moins un jour"); return;}

    
    if (window.confirm('Voulez-vous créer le cycle "' + name + '" ? \nValeurs : \nNom : ' + name + "\n" + "Vanne : " + id_ev + "\n" + "Heure début : " + StartHour.getHours() + "h" + StartHour.getMinutes() + "\n" + "Heure de fin : " + EndHour.getHours() + "h" + EndHour.getMinutes() + "\n" + "Jours : " + daysactive)) {
        $('#ModalCreateSchedule').modal('hide');
        AddCycle(name, id_ev, StartHour, EndHour, daysactive, 0);
    }    

    $('#ModalCreateSchedule').modal('hide');
});



// ----------------------------------------------------------------- OTHERS FUNCTIONS

$(document).ready(function() {
    getDate();
    InitValves();
    InitCycles();
});