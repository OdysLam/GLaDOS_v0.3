import appdaemon.appapi as appapi
from datetime import datetime

class Timer(appapi.AppDaemon):
    # Initialize will be called at startup or reload
    def initialize(self):
        self.log("initialize")
        self.countdown_handle = False
        self.timer_name = self.args["timer_name"]
        self.service_on = self.args["service_on"]
        self.service_off = self.args["service_off"]
        self.domain = self.args["on_off_domain"]
        self.timer_switch_entity = self.args["timer_switch_entity"]
        self.timer_handle_on = self.listen_event(self.set_timer_cb, "call_service",service_data = {"entity_id":[self.timer_switch_entity]}, domain = self.domain, service = self.service_on)
        service, kwargs = self.info_listen_event(self.timer_handle_on)
        self.log(service)
        self.log(kwargs)
        self.timer_handle_off = self.listen_event(self.end_timer_cb, "call_service",service_data = {"entity_id":[self.timer_switch_entity]}, domain = self.domain, service = self.service_off)
        self.set_state(self.timer_name, state = "00:00:00")
    def set_timer_cb(self, domain, service, entity_id):
        self.log("entered set_timer_cb")
        self.log(domain)
        self.log(service)
        self.log(entity_id)
        input_slider = self.args["minutes"]
        self.minutes = float(self.get_state( self.args["minutes"] ))
        self.log(self.minutes)
        self.time = int(self.minutes) * 60
        now = datetime.now()
        self.countdown_handle = self.run_every(self.countdown_cb, now, 1)
        hours = self.time / 3600
        minutes = (self.time % 3600) / 60
        seconds = (self.time % 3600) % 60
        time = str("%.2d" % hours) + ":" + str("%.2d" % minutes) + ":" + str("%.2d" % seconds)
        self.set_state(self.timer_name, state = time)
    def countdown_cb(self,now):
        if self.time == 0:
            self.terminate()
            return None 
        self.time -= 1
        hours = self.time / 3600
        minutes = (self.time % 3600) / 60
        seconds = (self.time % 3600) % 60
        time = str("%.2d" % hours) + ":" + str("%.2d" % minutes) + ":" + str("%.2d" % seconds)
        self.set_state(self.timer_name, state = time)

    def end_timer_cb(self,domain, service, entity_id):
        self.terminate()

    def terminate(self):
        # hass will listen for this custom event and turn_off switch.heater
        self.log("just ended")
        self.cancel_listen_event(self.timer_handle_off)
        self.turn_off(self.timer_switch_entity)
        self.timer_handle_off = self.listen_event(self.end_timer_cb, "call_service", domain = self.domain, service = self.service_off, entity_id = self.timer_switch_entity)
        self.cancel_timer(self.countdown_handle)
        self.countdown_handle = None
        self.set_state(self.timer_name, state = "00:00:00") 

    
    