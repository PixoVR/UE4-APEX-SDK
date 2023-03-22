
function loadDocMenu()
{
	var menu = $("#projectswitcher");
	menu.change(openDocPage);
	menu.attr("title","Choose a different documentation page.\n\nHold shift to open in a new tab.");

	var url = "../../../../../documentation/documentation/html/menu.json";

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
	var mainurl = "/";
	mainurl = "../../../../documentation/html";	//dev url

	menu.empty();

	if (status==="success")
	{
		//menu.append("<option value=''>Choose...</option>");
		menu.append("<option value='"+mainurl+"'>Main Menu</option>");
	}

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

function openDocPage(evt)
{
	//console.log(arguments);
	var shift = evt.shiftKey;
	var menu = $(this);
	var url = menu.val();

	if (url == '')
		return;

	menu.val( menu.data("selected") );

	console.log("will load: "+url);
	if (shift)
		window.open(url,'_blank');
	else
		window.open(url,'_self');
}

//load items when document is ready.
$( loadDocMenu );

