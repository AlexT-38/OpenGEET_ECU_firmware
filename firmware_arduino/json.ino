/* 
 *  see:  https://www.json.org/json-en.html for reference
 *  some sort of sensible system of names for these functions would make life easier 
 * 
 * a json file is a json object, 
 * objects are lists of labels and values
 * values can be string, number, object, array
 * labels are also strings, but are not values
 * arrays are lists of values
 * 
 * list can be on one line, or spread out over multiple lines, preferably indented
 * 
 * perhaps the most sensible way to go about this would be
 * to have a single function to add values to lists
 * and a single function to add label value pairs to objects
 * with the content of values being pre built String (class, not value or label)
 * 
 * but this means building the structure inside out.
 * and the streaming nature of logging requires sequential additions
 * 
 * function naming conventions:
 * start:                         json_
 * writes some characters:        json_wr_
 * adds an element:               json_add_
 * starts a collection:           json_open_
 * ends a collection:             json_close_
 * adds an element to a list:     json_append_
 * 
 * for the sake of simplicity, overloads will be used
 * adding/appending to an array is distinguished from adding/appending to an object by supplying a label
 * 
 * for the sake of brevity, labels should always be stored in flash
 */

#define JSON_TAB '\t'
#define JSON_MAX_DEPTH 255
byte json_depth;          // how many levels of recursion in the object structure
bool json_first;          //set true when entering into a new collection so that a comma is not added

//////////////////////////////////////////////////////////////////////////////////////////
// heiracy calls, no characters written

/* call to move down through the heirachy */
bool json_down()
{
  json_depth++;
  if(json_depth < JSON_MAX_DEPTH)
  {
    json_first = true;
    return true;
  }
  else 
  {
    json_depth--;        return false;
  }
}


/* call to move up the heirachy, to close a collection */
void json_up()
{
  if(json_depth) json_depth--;
  else {} //end of json file in this case
}


//////////////////////////////////////////////////////////////////////////////////////////
// basic elements

/* starts the json file and reset the heirachy tracker. finish with json_close_object */
void json_wr_start(String &dst)
{
  dst = "{";
  json_depth = 1;
  json_first = true;
}

/* new line, add tabs to depth*/
void json_wr_nl(String &dst)
{
  dst += "\n";
  if (JSON_TAB)
  {
    char tabs[json_depth+1];
    memset(tabs,JSON_TAB,json_depth);
    tabs[json_depth] = 0;
    dst += tabs;
  }
}

/* add a new entry to the current collection */
void json_wr_list(String &dst)
{
  if(!json_first)  {     dst += ",";   } //add list separator if not first
  else {    json_first = false;  }
}

/* add a new entry to the current collection on a new line */
void json_wr_append(String &dst)
{
  json_wr_list(dst);
  json_wr_nl(dst);
}



//////////////////////////////////////////////////////////////////////////////////////////
// add values (inc labels)

/* add a label */
void json_add_label(String &dst, const char *label)
{
  json_add(dst, label);
  dst += ":";
}
void json_add_label(String &dst, const __FlashStringHelper *label_pgm)
{
  json_add(dst, label_pgm);
  dst += ":";
}



/* add an integer value */
void json_add(String &dst, int value)
{
  dst += value;
  json_first = false;
}
void json_add(String &dst, unsigned int value)
{
  dst += value;
  json_first = false;
}
void json_add(String &dst, long value)
{
  dst += value;
  json_first = false;
}
void json_add(String &dst, unsigned long value)
{
  dst += value;
  json_first = false;
}

/* add a string from sram */
void json_add(String &dst, const char *string)
{
  dst += "\"";
  dst += string;
  dst += "\"";
  json_first = false;
}
/* append a string from progmem */
void json_add(String &dst, const __FlashStringHelper *string)
{
  dst += "\"";
  dst += string;
  dst += "\"";
  json_first = false;
}


//////////////////////////////////////////////////////////////////////////////////////////
// array elements 

//typically arrays will either be of values all on one line, 
// or of objects, each 'append'ed to the list with json_append()

/* start a new array, follow with {json_wr_list, opt. new line, value} (number, string, array or object)*/
void json_open_array(String &dst)
{
  dst += "[";
  json_first = true;
  json_down();
}
/* end the current array */
void json_close_array(String &dst)
{
  json_up();
  dst += "]";
  json_first = false;
}
void json_append_close_array(String &dst)
{
  json_up();
  json_wr_nl(dst);
  dst += "]";
  json_first = false;
}


/* add an array and populate with numbers */
void json_add_array(String &dst, int * values, unsigned int count)
{
  json_open_array(dst);
  while(count--)
  {
    json_wr_list(dst);
    json_add(dst,*values++);
  }
  json_close_array(dst);
}
void json_add_array(String &dst, unsigned int * values, unsigned int count)
{
  json_open_array(dst);
  while(count--)
  {
    json_wr_list(dst);
    json_add(dst,*values++);
  }
  json_close_array(dst);
}




////////////////////////////////////////////
// objects

/* start an object */
void json_open_object(String &dst)
{
  dst += "{";
  json_first = true;
  json_down();
}
/* end the current object */
void json_close_object(String &dst)
{
  json_up();
  dst += "}";
  json_first = false;
}
void json_append_close_object(String &dst)
{
  json_up();
  json_wr_nl(dst);
  dst += "}";
  json_first = false;
}

/* add an entry to an object on a new line with the given label */
void json_append_label(String &dst, const char * label)
{
  json_wr_append(dst);
  json_add_label(dst, label);
}
/* as above, label in prgmem */
void json_append_label(String &dst, const __FlashStringHelper * label)
{
  json_wr_append(dst);
  json_add_label(dst, label);
}






///////////////////////////////////////////////////////////////////////////////////////
// append values to objects and arrays
// 
// if label is supplied, adds to object, otherwise adds to array
//



// Append Object ///////////////////////////////////////////////////////////////////////////////////////
/* add an object to an array on a new line */
void json_append_obj(String &dst)
{
  json_wr_append(dst);
  json_open_object(dst);
}
/* add an object to an object with the given label on a new line */
void json_append_obj(String &dst, const __FlashStringHelper * label )
{
  json_append_label(dst, label);
  json_open_object(dst);
}



// Append array ///////////////////////////////////////////////////////////////////////////////////////
/* add an object to an array on a new line */
void json_append_arr(String &dst)
{
  json_wr_append(dst);
  json_open_array(dst);
}
/* add an object to an object with the given label on a new line */
void json_append_arr(String &dst, const __FlashStringHelper * label )
{
  json_append_label(dst, label);
  json_open_array(dst);
}

// Append int array ///////////////////////////////////////////////////////////////////////////////////////
void json_append_arr(String &dst, int * values, unsigned int count)
{
  json_wr_append(dst);
  json_add_array(dst, values,  count);
}
/* add an object to an object with the given label on a new line */
void json_append_arr(String &dst, const __FlashStringHelper * label, int * values, unsigned int count )
{
  json_append_label(dst, label);
  json_add_array(dst,  values,  count);
}
//unsigned overloads
void json_append_arr(String &dst, unsigned int * values, unsigned int count)
{
  json_wr_append(dst);
  json_add_array(dst, values,  count);
}
/* add an object to an object with the given label on a new line */
void json_append_arr(String &dst, const __FlashStringHelper * label, unsigned int * values, unsigned int count )
{
  json_append_label(dst, label);
  json_add_array(dst,  values,  count);
}

// Append strings  ///////////////////////////////////////////////////////////////////////////////////////
/* add an entry to the current object with the given label, for ints and strings with either label or sting in progmem */
void json_append(String &dst, const char* label, const char* value) //string
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const char* label, const __FlashStringHelper* value) //_string
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const __FlashStringHelper* label, const char* value) //_string
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const __FlashStringHelper* label, const __FlashStringHelper* value) //_string
{
  json_append_label(dst, label);
  json_add(dst, value);
}
//append strings to arrays
void json_append(String &dst, const char* value) //_string
{
  json_wr_append(dst);
  json_add(dst, value);
}
void json_append(String &dst, const __FlashStringHelper* value) //_string
{
  json_wr_append(dst);
  json_add(dst, value);
}

// Append integers //////////////////////////////////////////////////////////////////////////////////////////////
void json_append(String &dst, const char* label, int value) //_int
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const __FlashStringHelper* label, int value) //_int
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const char* label, long value) //_int
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const __FlashStringHelper* label, long value) //_int
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const char* label, unsigned int value) //_int
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const __FlashStringHelper* label, unsigned int value) //_int
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const char* label, unsigned long value) //_int
{
  json_append_label(dst, label);
  json_add(dst, value);
}
void json_append(String &dst, const __FlashStringHelper* label, unsigned long value) //_int
{
  json_append_label(dst, label);
  json_add(dst, value);
}

//integers shouldn't be appended to arrays. use json_append_arr(&dst,*values,count) instead
