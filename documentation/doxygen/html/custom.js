$(document).ready(function()
{
	$(".directory > tbody > tr > .desc").addClass("hide");
//	addBreaks();

	makeResponsive();

	$(".entry").on("change", "input[data-toggle='toggle']", function() {
		$(this).parents(".entry").next(".hide").toggle();
	})
});

$(window).resize( function()
{
	makeResponsive();
});

/*
function addBreaks() {

	if ($(".fragment").length) {
		$(".fragment").html($(".fragment").html()
		.replace(/,/g , ',&#8203;').replace(/\(/g , '(&#8203;'));
	}

	if ($("code > a.el").length) {
		$("code > a.el").html($("code > a.el").html().replace(/\//g , '/&#8203;'));
	}
}
*/

function makeResponsive()
{
	//return;

	if ($(window).width() > 500) {
	//	$(".directory > tbody > tr > td:first-child > label").remove();
	//	$(".hide").css("display", "table-cell");
	} else {
		// Make dropdown when item has a brief desc.
		if (!$(".hide:not(:empty)").prev("td").find("input").length) {
			$(".hide:not(:empty)").prev(".entry").append("<label class='desc-toggle'><input type='checkbox' data-toggle='toggle'>(brief)</label>");
			$('.hide').css("display", "none");
		}
	}

	if ($(window).width() < 768) {
		$("#side-nav").removeClass("side-nav-resizable");
		
	}
}

/*
// Overrides gotoNode in navtree.js to close the nav when going to a new page.
function gotoNode(o,subIndex,root,hash,relpath) {

  var nti = navTreeSubIndices[subIndex][root+hash];
  o.breadcrumbs = $.extend(true, [], nti ? nti : navTreeSubIndices[subIndex][root]);
  if (!o.breadcrumbs && root!=NAVTREE[0][1]) { // fallback: show index
  	$('#nav-tree-contents li:first > .item > a').click();
    navTo(o,NAVTREE[0][1],"",relpath);
    $('.item').removeClass('selected');
    $('.item').removeAttr('id');
  }
  if (o.breadcrumbs) {
    o.breadcrumbs.unshift(0); // add 0 for root node
    showNode(o, o.node, 0, hash);
  }

	// Override: close the nav.
//	if ($(window).width() <= 768)
//		$('#nav-tree-contents li:first > .item > a').click();
}
*/
