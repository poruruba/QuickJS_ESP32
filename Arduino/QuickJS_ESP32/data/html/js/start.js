'use strict';

//const vConsole = new VConsole();
//window.datgui = new dat.GUI();

var vue_options = {
    el: "#top",
    mixins: [mixins_bootstrap],
    data: {
    	quickjs_url_base: "",
    	input_text: "",
    	files: [],
    	downloaded_js: "",
    	console_text: "",
    },
    computed: {
    },
    methods: {
		read_file: function(files){
			if (files.length <= 0) {
				return;
			}
			this.files = files;
		},
		copy_to_texearea: function(){
			this.input_text = this.downloaded_js;
		},
    	upload_textarea: async function(){
    		try{
	    		var blob = new Blob([this.input_text], { type: "application/javascript" });
		    	var result = await do_post_formdata(this.quickjs_url_base + "/upload_js", {}, blob);
		    	alert("アップロードしました。");
		    }catch(error){
		    	console.error(error);
		    	alert(error);
		    }
    	},
    	upload_file: async function(){
    		if( this.files.length <= 0 ){
    			alert("ファイルを選択してください。");
    			return;
    		}
    		
    		try{
		    	var result = await do_post_formdata(this.quickjs_url_base + "/upload_js", {}, this.files[0]);
		    	alert("アップロードしました。");
		    }catch(error){
		    	console.error(error);
		    	alert(error);
		    }
    	},
    	download_text: async function(){
			try{
	    		var result = await do_get(this.quickjs_url_base + "/download_js", {});
	    		this.downloaded_js = result;
	    		this.dialog_open("#quickjs_js_dialog");
		    }catch(error){
		    	console.error(error);
		    	alert(error);
		    }
    	},
    	
    	save_text: function(text){
	    	const url = URL.createObjectURL(new Blob([text], { type: "text/plain" }));
			const a = document.createElement("a");
			a.download = 'notitle.txt';
			a.href = url;
			a.click();
			a.remove();
			URL.revokeObjectURL(url);
    	},
    	save_textarea: function(){
    		this.save_text(this.input_text);
    	},
    	save_getfile: function(text){
    		this.save_text(this.downloaded_js);
    	},
		catch_tabKey: function( e ){
			// Ctrl-sを無視
 			if (e.keyCode == 83 && e.ctrlKey){
				e.preventDefault();
				if( !confirm("アップロードしますか？") )
	 				return;
	 			this.save_textarea();
 			}
			// TABのみフック
			if( e.keyCode != 9 )
				return;
 
			e.preventDefault();

			var cursorPosition = e.target.selectionStart;
			var cursorLeft     = e.target.value.substr( 0, cursorPosition );
			var cursorRight    = e.target.value.substr( cursorPosition, e.target.value.length );

			e.target.value = cursorLeft+"\t"+cursorRight;
			e.target.selectionEnd = cursorPosition + 1;
		},
    },
    created: function(){
    },
    mounted: function(){
        proc_load();

    }
};
vue_add_data(vue_options, { progress_title: '' }); // for progress-dialog
vue_add_global_components(components_bootstrap);
vue_add_global_components(components_utils);

/* add additional components */

window.vue = new Vue( vue_options );

function do_post_formdata(url, params, file){
    var body = Object.entries(params).reduce( (l, [k,v])=> { l.append(k, v); return l; }, new FormData());
    body.append("file", file);

  return fetch(url, {
      method : 'POST',
      body: body,
  })
  .then((response) => {
    if( !response.ok )
      throw 'status is not 200';
    return response.text();
  });
}

function do_get(url, qs) {
  var params = new URLSearchParams(qs);

  var params_str = params.toString();
  var postfix = (params_str == "") ? "" : ((url.indexOf('?') >= 0) ? ('&' + params_str) : ('?' + params_str));
  return fetch(url + postfix, {
      method: 'GET',
    })
    .then((response) => {
      if (!response.ok)
        throw 'status is not 200';
      return response.text();
    });
}
