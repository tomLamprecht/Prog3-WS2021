import { Component, EventEmitter, Input, Output} from '@angular/core';
import { ItemModel } from 'src/lib/data-access/models/itemModel';


@Component({
  selector: 'column',
  templateUrl: './column.component.html',
  styleUrls: ['./column.component.scss'],
})
export class ColumnComponent{
  @Input()
  title = 'column'

  @Input()
  columnId : string;

  @Input()
  items: ItemModel[];

  @Output()
  deleteColumn = new EventEmitter();

  @Output()
  changeColumnTitle = new EventEmitter<string>();

  createEmptyItem(){
    return {
      id: (Math.random()*100),
      title: "",
      position: 0,
      timestamp: ""
    };
  }

  addEmptyItem(){
      this.items.push(this.createEmptyItem());

  }

  ngAfterViewInit(){

    let element  = document.getElementById(this.columnId);

    if(element != null &&  element.className == "inputColumn")
      element.focus();
    else{
      console.log("Couldnt find Column of id" + this.columnId);
    }

  }


  changeItemName(item, event){
    item.title = event;
  }

  onDelete(){
    this.deleteColumn.emit();
  }

  onChange(event){
    this.changeColumnTitle.emit(event.target.value);
  }

  deleteItem(item){
    var index = this.items.indexOf(item);
    this.items.splice(index,1);
  }

  showDeleteButton: boolean = false;
}


