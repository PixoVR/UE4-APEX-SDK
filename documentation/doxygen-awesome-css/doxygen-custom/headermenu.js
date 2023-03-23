
//yeah, a global variable.  Sorry.
var headerShiftKey = false;

function loadDocMenu()
{
	var menu = $("#projectswitcher");
	menu.on("mousedown",checkShift);
	//$(document).on("keydown keyup",checkShift);
	//menu.on("keydown keyup",checkShift);
	menu.change(openDocPage);
	menu.attr("title","Choose a different documentation page.\nHold shift to open in a new tab.");

	var url = "/menu.json";

	if (!window.location.host.includes("pixovr"))
		url = "../../../../../documentation/documentation/html/menu.json";

	$.ajax({
		dataType: "json",
		url: url,
		/* data: data, */
		success: buildDocMenu,
		error: buildDocMenuError
	});
}

function buildDocMenu(items,status,xhr)
{
	var menu = $("#projectswitcher");
	var title = document.title;

	menu.empty();

	//console.log(items);
	items.forEach( option => {
		var o = $("<option>");
		o.attr("value",option.url);
		if (title.includes(option.name))
		{	o.attr("selected","");
			menu.data("selected",option.url);
		}
		o.text(option.name);
		menu.append( o );
	} );
}

function buildDocMenuError(xhr,status,errorThrown)
{
	console.error( arguments );

	var menu = $("#projectswitcher");
	menu.css("color","red");

	var items = [
		{
			name: "Error loading menu",
			url: ''
		}
	]

	buildDocMenu(items,status)
}

function checkShift(evt)
{
	//console.log("shift: "+evt.shiftKey);
	headerShiftKey = evt.shiftKey;
}

function openDocPage(evt)
{
	//console.log(arguments);

	var menu = $(this);
	var url = menu.val();

	if (url == '')
		return;

	menu.val( menu.data("selected") );

	//console.log("will load: "+url);
	if (headerShiftKey)
		window.open(url,'_blank');
	else
		window.open(url,'_self');

	headerShiftKey = false;
}

//load items when document is ready.
$( loadDocMenu );

