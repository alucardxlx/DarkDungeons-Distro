/* $Id: crafting.inc 1292 2008-09-08 20:36:37Z AustinHeilman $ */

use uo;
use os;
use cfgfile;

include ":containers:containers";
include ":itemUtils:itemdesc";

/* GetToolOnMaterialUses(tool, material)
 *
 * Retrieves an config elem of what can be crafted with a combination of a tool on a material.
 *
 * Parameters
 * tool: 	Item that the crafting use script is bound to or an object type.
 * material:	
 *              If an item is passed, it will first check its object type for a match with the tool, then graphic.
 *              If a number is passed, it will be handled as if it was a graphic.
 *
 * Return Value:
 * Returns a config elem on success.
 * Returns 0 if nothing can be crafted with the combination used.
 *
 */
function GetInitialMenuName(tool, material)
	var search;
	var cfg_elem;
	var cfg_file := ReadConfigFile(":*:toolOnMaterial");
	if ( cfg_file.errortext )
		SysLog("GetInitialMenu() - Could not open :*:toolOnMaterial - "+cfg_file.errortext);
		return 0;
	endif
		
	if ( tool.IsA(POLCLASS_ITEM) )
		search := "Tool=" + Hex(tool.objtype);
	else
		search := "Tool=" + Hex(CInt(tool));
	endif
	search += "&Material=";
	
	if ( material.IsA(POLCLASS_ITEM) )
		cfg_elem := cfg_file[search+Hex(material.objtype)];
		if ( cfg_elem )
			return cfg_elem.ShowMenu;
		else
			material := material.graphic;
		endif
	endif 
	
	cfg_elem := cfg_file[search+Hex(CInt(material))];
	if ( cfg_elem )
		return cfg_elem.ShowMenu;
	endif
		
	return 0;
endfunction

/* GetInitialMenusByTool(tool)
 *
 * Returns.. something, haven't decided yet!
 *
 * Parameters
 * tool: 	Item reference or object type of the tool being used.
 *
 * Return Value:
 * returns an array of config elems.
 *
 */
function GetInitialMenusByTool(tool)
	var search;
	
	if ( tool.IsA(POLCLASS_ITEM) )
		search := Lower("Tool=" + Hex(tool.objtype));
	else
		search := Lower("Tool=" + Hex(CInt(tool)));
	endif
	
	var size := Len(search);
	
	var cfg := ReadConfigFile(":*:toolOnMaterial");
	
	var elem_list := array{};
	foreach elem_name in ( GetConfigStringKeys(cfg) )
		if ( Lower(elem_name[0, size]) == search )
			elem_list.Append(cfg[elem_name]);
		endif
		SleepMS(2);
	endforeach

	return elem_list;
endfunction

/* ShowCraftablesMenu(mobile, cur_menu_elem, level, material.color);
 *
 * Receives an elem containing submenu properties and craft item properties.
 * Displays them using the left-right slider menu in the UO client.
 *
 * Parameters
 * mobile: 	Player that will be interacting with the menu.
 * menu_elem:	Config elem that will control the options in the menu.
 * depth:	>1 will have a back button.
 * color:	Optional argument to make the items (not submenus) colored.
 *
 * Return Value:
 * Returns 0 if the menu was not displayed or was cancelled.
 * Returns a struct containing either a SubMenu property or an objecttype to craft.
 *
 */
function ShowCraftablesMenu(mobile, menu_elem, depth, color:=0)
	var menu_selection := struct;
	menu_selection.+objtype;
	menu_selection.+SubMenu;
	
	var menu_options := array;
	
	var menu_obj := CreateMenu("What would you like to make?");
	
	if ( depth > 1 )
		menu_selection.objtype := 0;
		menu_selection.SubMenu := -1;
		menu_options.Append(menu_selection);
		
		AddMenuItem(menu_obj, 8304, "^- Back");
	endif
	
	var menu_list := GetConfigStringArray(menu_elem, "SubMenu");
	foreach submenu in ( menu_list )
		menu_selection.objtype := 0;
		menu_selection.SubMenu := 0;
		
		var line := SplitWords(submenu);
		var graphic := GetObjTypeGraphic(CInt(line[2]));

		var item_name := line[3];
		while( item_name["_"] )
			item_name["_"] := " ";
		endwhile

		menu_selection.SubMenu := line[1];
		menu_options.Append(menu_selection);
		
		AddMenuItem(menu_obj, graphic, item_name+" (submenu)");
		sleepms(2);
	endforeach
	
	var item_list := GetConfigStringArray(menu_elem, "Entry");
	foreach entry in ( item_list )
		menu_selection.objtype := 0;
		menu_selection.SubMenu := 0;
		
		var line := SplitWords(entry);
		
		var objtype := GetObjTypeByName(line[1]);
		var menu_name;
		if ( objtype )
			menu_name := GetObjTypeDesc(objtype);
		else
			objtype := CInt(line[1]);
			menu_name := CStr(line[2]);
		endif
		
		var graphic := GetObjTypeGraphic(objtype);
				
		while( menu_name["_"] )
			menu_name["_"] := " ";
		endwhile
		
		menu_selection.objtype := objtype;
		menu_options.Append(menu_selection);
		
		AddMenuItem(menu_obj, graphic, menu_name, color);
		sleepms(2);
		
		sleepms(2);
	endforeach
	
	if ( (menu_list.size() < 1) && (item_list.size() < 1) )
		SendSysMessage(mobile, "There are no submenus or items listed here.");
		return 0;
	endif
	
	var input := SelectMenuItem2(mobile, menu_obj);
	return menu_options[input.index];
endfunction

/* CheckForAdditionalTools(mobile, tool_list)
 *
 * Determines if additional tools such as forges, anvils, etc, are available.
 * Checks the player's pack, as well as dynamic and static items surrounding the player.
 *
 * Parameters
 * mobile: 	Mobile performing the crafting.
 * tool_list:	Array containing object types of possible matches.
 *
 * Return Value:
 * Returns 1 if something valid was found.
 * Returns 0 if nothing could be found.
 *
 */
function FindTool(mobile, tool_list)
	foreach item in ( EnumerateItemsInContainer(mobile.backpack) )
		if ( CStr(item.objtype) in tool_list )
			return item;
		endif
	endforeach
	
	foreach item in ( ListItemsNearLocation(mobile.x, mobile.y, mobile.z, 5, mobile.realm) )
		if ( CheckLineOfSight(mobile, item) )
			if ( CStr(item.objtype) in tool_list )
				return item;
			endif
		endif
		sleepms(2);
	endforeach
	
	foreach item in ( ListStaticsNearLocation(mobile.x, mobile.y, mobile.z, 5, 0, mobile.realm) )
		if ( CheckLineOfSight(mobile, item) )
			if ( CStr(item.objtype) in tool_list )
				return item;
			endif
		endif
		sleepms(2);
	endforeach
	
	return 0;
endfunction