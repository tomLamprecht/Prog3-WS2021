import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';

import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { ToolbarComponent } from '../lib/ui/toolbar/toolbar.component';
import { BoardComponent } from 'src/lib/ui/board/board.component';
import { ColumnComponent } from 'src/lib/ui/column/column.component';
import { ItemComponent } from 'src/lib/ui/item/item.component';

@NgModule({
  declarations: [AppComponent, ToolbarComponent, BoardComponent, ColumnComponent, ItemComponent],
  imports: [BrowserModule, AppRoutingModule],
  providers: [],
  bootstrap: [AppComponent],
})
export class AppModule {}


