var setup_finished = false;
async function call_setup(){
  try{
    await setup();
  }catch(error){
    console.log(error);
  }finally{
    setup_finished = true;
  }
}

var loop_running = false;
async function check_and_call_loop(){
  if ( !setup_finished )
    return;
  if ( !loop_running ){
    loop_running = true;
    try{
      await loop();
    }catch(error){
      console.log(error);       
    }finally{
      loop_running = false;
    }
  }
}

if (setup && typeof setup === 'function')
  call_setup();
else
  setup_finished = true;

if (loop && typeof loop === 'function')
  esp32.setLoop(check_and_call_loop);
