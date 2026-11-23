import { Component } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { ZToastOutlet } from 'zenit-ui';

@Component({
  imports: [RouterOutlet, ZToastOutlet],
  selector: 'app-root',
  templateUrl: './app.html',
})
export class App {}
